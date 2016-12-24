/* File: pa_usart.c
 *
 * Protocol Analysis routines for Async Serial
 *
 * Author: Jack Bradach <jack@bradach.net>
 *
 * Copyright (C) 2016 Jack Bradach <jack@bradach.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "pa_usart.h"
#include "proto.h"

#define DEFAULT_SYMBOL_LENGTH 8
#define DEFAULT_PARITY USART_PARITY_NONE
#define DEFAULT_STOP_BITS USART_STOP_BITS_ONE

#define MAX_SAMPLE_WIDTH 32
#define MAX_DESC_LEN 64
#define USART_FLAG_MASK (SPI_FLAG_CPOL | SPI_FLAG_CPHA | SPI_FLAG_ENDIANESS | SPI_FLAG_CS_POLARITY)


enum pa_usart_sm {
    USART_SM_IDLE = 0,
    USART_SM_SOF,
    USART_SM_DATA,
    USART_SM_EOF
};

enum {
    USART_PA_SPACE = 0,
    USART_PA_MARK
};

/* Struct: pa_usart_state
 *
 * State structure for USART decoder
 *
 * Fields:
 *  data - shift register for incoming data
 *  bits_sampled - counter for incoming bits
 *  last_transition - number of samples since last transition; this should
 *      be used with the sample rate to figure out how long a bit is.
 */
struct pa_usart_state {
    uint16_t data;
    uint32_t bit_width;
    uint32_t bit_frac_cnt;
    uint8_t nbits_sampled;
    uint8_t data_valid;
    int sm;
};

/* Struct: pa_usart_ctx
 *
 * Context structure for the USART decoder; this allows multiple
 * USART decoders to be independently running.  They must be
 * initialized using the helper functions.
 *
 * Fields:
 *      mask_usart - one-hot sample mask for USART line
 *      symbol_length - payload length
 *      parity - EVEN, ODD, or NONE
 *      bit_time - time between bits in microseconds.  This will be
 *                 calculated automatically by default.
 *      state - USART decoder state machine vars
 */
struct pa_usart_ctx {
    uint32_t mask_usart;
    uint8_t symbol_length;
    enum usart_parity parity;
    float sample_period;
    proto_t *pr;
    struct pa_usart_state *state;
    uint64_t sample_cnt;
    uint64_t decode_cnt;
    struct timespec elapsed;
    char *desc;
};

static inline uint8_t unswizzle_sample(struct pa_usart_ctx *ctx, uint32_t sample);
static void stream_decoder(struct pa_usart_ctx *ctx, uint8_t sample);

static void usart_add_dframe_sof(struct pa_usart_ctx *c, uint64_t idx);
static void usart_add_dframe_data(struct pa_usart_ctx *c, uint64_t idx, uint8_t data);
static void usart_add_dframe_eof(struct pa_usart_ctx *c, uint64_t idx);
static void usart_add_dframe_error(struct pa_usart_ctx *c, uint64_t idx);

static struct timespec ts_diff(struct timespec *start, struct timespec *end);
static struct timespec ts_add(struct timespec *a, struct timespec *b);

/* Function: pa_usart_decode_stream
 *
 * Public interface to the USART stream decoder; it takes a raw digital,
 * unswizzles the USART signals that the context is registered to handle,
 * and then passes it to the stream decoder.
 *
 * Parameters:
 *      ctx - Handle to a USART decode context.
 *      raw - 32-bit raw sample from logic analyzer
 *
 */
void pa_usart_decode_stream(struct pa_usart_ctx *ctx, uint32_t raw)
{
    uint8_t sample = unswizzle_sample(ctx, raw);
    stream_decoder(ctx, sample);
}

/* Function: pa_usart_decode_chunk
 *
 * Public interface to the USART chunk decoder; same idea as them
 * stream but it works on all samples in a capture.
 *
 * Parameters:
 *      ctx - Handle to a USART decode context.
 *      cap - cap_t handle (analog or digital)
 */
// TODO - 2016/12/21 - jbradach - this needs to track sample index
// TODO - separate from the cap file; multiple cap files can be
// TODO - "stiched" together and ctx needs an 'absolute' index
// TODO - based on how many samples have been seen since last reset.
void pa_usart_decode_chunk(struct pa_usart_ctx *ctx, cap_t *cap)
{
    cap_digital_t *dcap;
    struct timespec ts_start, ts_end, ts_delta;

    dcap = cap_get_digital(cap);

    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    for (uint64_t i = 0; i < cap_get_nsamples((cap_t *) dcap); i++) {
        uint32_t dsample = cap_digital_get_sample(dcap, i);
        uint8_t us = unswizzle_sample(ctx, dsample);
        stream_decoder(ctx, us);
    }
    clock_gettime(CLOCK_MONOTONIC, &ts_end);

    ts_delta = ts_diff(&ts_start, &ts_end);

    ctx->elapsed = ts_add(&ctx->elapsed, &ts_delta);
}

static struct timespec ts_add(struct timespec *a, struct timespec *b)
{
    struct timespec tmp;

    tmp.tv_sec = a->tv_sec + b->tv_sec;
    tmp.tv_nsec = a->tv_nsec + b->tv_nsec;
    if (tmp.tv_nsec >= 1E9) {
        tmp.tv_nsec -= 1E9;
        tmp.tv_sec++;
    }
    return tmp;
}

static struct timespec ts_diff(struct timespec *start, struct timespec *end)
{
    struct timespec tmp;

    if ((end->tv_nsec - start->tv_nsec) < 0) {
		tmp.tv_sec = end->tv_sec - start->tv_sec - 1;
		tmp.tv_nsec = 1E9 + (end->tv_nsec - start->tv_nsec);
	} else {
		tmp.tv_sec = end->tv_sec - start->tv_sec;
		tmp.tv_nsec = end->tv_nsec - start->tv_nsec;
	}
    return tmp;
}

/* Returns the number of seconds (as a double) consumed
 * by the protocol decoder since the last time it was reset.
 */
double pa_usart_time_elapsed(pa_usart_ctx_t *ctx)
{
    double elapsed;
    struct timespec *t = &ctx->elapsed;
    elapsed = t->tv_nsec * 1.0E-9;
    if (t->tv_sec) {
        elapsed += (t->tv_sec * 1E9);
    }
    return elapsed;
}

/* Function: unswizzle_sample
 *
 * Maps signals from a raw analyzer output (up to 32-bits wide) to a
 * single bit sample.
 *
 * Parameters:
 *      ctx - Valid handle to USART decoder context
 *      sample - raw digital sample data
 *
 * Returns:
 *      1-bit representation of USART sample
 */
static inline uint8_t unswizzle_sample(struct pa_usart_ctx *ctx, uint32_t sample)
{
    uint8_t unswizzled = 0;

    if (sample & ctx->mask_usart)
        unswizzled |= 1;

    return unswizzled;
}

/* Function: sm_idle
 *
 * USART Decoder State Machine: Idle
 */
static inline void sm_idle(struct pa_usart_ctx *ctx, uint8_t sample)
{
    struct pa_usart_state *state = ctx->state;
    if (USART_PA_SPACE == sample) {
        usart_add_dframe_sof(ctx, ctx->sample_cnt);
        state->sm = USART_SM_SOF;
        state->nbits_sampled = 0;
        // XXX - autobaud disabled
        // state->bit_width = 1;
        state->bit_frac_cnt = 1;
        state->data = 0;
    }
}

/* Function: sm_sof
 *
 * USART Decoder State Machine: Start of Frame
 */
static inline void sm_sof(struct pa_usart_ctx *ctx, uint8_t sample)
{
    struct pa_usart_state *state = ctx->state;

    if (USART_PA_MARK == sample) {
        /* A start bit is valid as long as it's at least half the bit
         * width.  If not, it's a spurious start; go back to idle.
         */
        if (state->bit_frac_cnt >= (state->bit_width / 2)) {
            state->sm = USART_SM_DATA;
            state->bit_frac_cnt = 1;
        } else {
            usart_add_dframe_error(ctx, ctx->sample_cnt);
            state->sm = USART_SM_IDLE;
        }
    } else if (state->bit_frac_cnt == state->bit_width) {
        /* Bit time expired */
        state->sm = USART_SM_DATA;
        state->bit_frac_cnt = 1;
    } else {
        /* TICK! */
        state->bit_frac_cnt++;
    }

    state->data = 0;
}

/* Function: sm_data
 *
 * USART Decoder State Machine: Data bit sampling
 */
static inline void sm_data(struct pa_usart_ctx *ctx, uint8_t sample)
{
    struct pa_usart_state *state = ctx->state;

    /* Sample as close to the middle of the bit as we can. */
    if (state->bit_frac_cnt == (state->bit_width / 2)) {
        state->data >>= 1;
        state->data |= (sample << (ctx->symbol_length - 1));
        state->nbits_sampled++;
        state->bit_frac_cnt++;
    } else if (state->bit_frac_cnt >= state->bit_width) {
        /* If we haven't sampled enough bits to make a symbol,
         * get ready for the next one; otherwise we're watching
         * for the stop bit.
         */
        if (state->nbits_sampled == ctx->symbol_length) {
            usart_add_dframe_eof(ctx, ctx->sample_cnt);
            state->sm = USART_SM_EOF;
        }
        state->bit_frac_cnt = 1;
    } else {
        /* TICK! */
        state->bit_frac_cnt++;
    }
}

/* Function: sm_data
 *
 * USART Decoder State Machine: End-of-Frame stop bit detection
 */
static inline void sm_eof(struct pa_usart_ctx *ctx, uint8_t sample)
{
    struct pa_usart_state *state = ctx->state;

    if (state->bit_frac_cnt == state->bit_width) {
        state->sm = USART_SM_IDLE;
        state->data_valid = 1;
    } else if (state->bit_frac_cnt == (state->bit_width / 2)) {
        // TODO - jbradach - 2016/12/11 - Report framing error to caller
        // TODO - so they can discard the data or calculate how far out
        // TODO - of spec it was.  Kick back to SOF and try to go from there.
        if (USART_PA_MARK != sample) {
            usart_add_dframe_error(ctx, ctx->sample_cnt);
            state->sm = USART_SM_SOF;
            state->bit_frac_cnt = 0;
        }
    } else if (state->bit_frac_cnt > (state->bit_width / 2)) {
        /* If the stop bit is truncated by a space before it's seen as
         * valid, report a framing error.  It's still a valid SOF for
         * the next frame, though.
         */
         if (USART_PA_SPACE == sample) {
            usart_add_dframe_sof(ctx, ctx->sample_cnt);
            state->sm = USART_SM_SOF;
            state->data_valid = 1;
            state->bit_frac_cnt = 0;
        }
    }
    state->bit_frac_cnt++;
}

/* Function: stream_decoder
 *
 * Stateful decoding of a USART stream.
 *
 * Parameters:
 *      ctx - handle to an initialized USART decoder context
 *      sample - a 1-bit sample, unswizzled using unswizzle_sample.
 * FIXME - 2016/12/10 - jbradach - Autobaud doesn't work yet because my plan
 * FIXME - to just measure the start-bit width only works if the first bit
 * FIXME - is a mark.  If it's a space (or multiple spaces!), the timing
 * FIXME - measurement does the wrong thing.  Right way to fix it is to
 * FIXME - start measuring frames to find a single-bit transition.  Maybe
 * FIXME - measure drift over time and report it?
 */
static void stream_decoder(struct pa_usart_ctx *ctx, uint8_t sample)
{
    struct pa_usart_state *state = ctx->state;

    switch (state->sm) {
        case USART_SM_IDLE:
            sm_idle(ctx, sample);
            break;

        case USART_SM_SOF:
            sm_sof(ctx, sample);
            break;

        case USART_SM_DATA:
            sm_data(ctx, sample);
            break;

        case USART_SM_EOF:
            sm_eof(ctx, sample);
            break;

        default:
            printf("State lost, resetting!\n");
            state->sm = USART_SM_DATA;
    }

    if (state->data_valid) {
        usart_add_dframe_data(ctx, ctx->sample_cnt, state->data);
        ctx->decode_cnt++;
        state->data_valid = 0;
        state->data = 0;
        state->nbits_sampled = 0;
    }

    ctx->sample_cnt++;
}

/* Function: pa_usart_ctx_init
 *
 * Frees the resources associated with a SPI Decode Context structure.
 * This needs to be called to avoid memory leaks when a decoder is no
 * longer needed.
 *
 * Parameters:
 *      ctx - handle SPI decode context structure to destroy; this handle
 *            will be immediately invalid after return.
 */
int pa_usart_ctx_init(struct pa_usart_ctx **new_ctx)
{
    struct pa_usart_ctx *ctx;
    proto_t *pr;

    if (NULL == new_ctx)
        return -EINVAL;

    /* Calloc takes care of most needed initialization, but we
     * set the symbol length to a sane default.
     */
    ctx = calloc(1, sizeof(struct pa_usart_ctx));
    ctx->symbol_length = DEFAULT_SYMBOL_LENGTH;
    ctx->state = calloc(1, sizeof(struct pa_usart_state));

    pr = proto_create();
    proto_set_note(pr, "USART");
    ctx->pr = pr;

    *new_ctx = ctx;
    return 0;
}

/* Function: pa_usart_cleanup_ctx
 *
 * Frees the resources associated with a SPI Decode Context structure.
 * This needs to be called to avoid memory leaks when a decoder is no
 * longer needed.
 *
 * Parameters:
 *      ctx - handle USART decode context structure to destroy; this handle
 *            will be immediately invalid after return.
 */
void pa_usart_ctx_cleanup(struct pa_usart_ctx *ctx)
{
    if (NULL == ctx)
        return;

    if (NULL != ctx->state) {
        free(ctx->state);
        ctx->state = 0;
    }

    if (ctx->pr) {
        proto_dropref(ctx->pr);
    }

    free(ctx);
}

static void usart_add_dframe_sof(struct pa_usart_ctx *c, uint64_t idx)
{
    proto_add_dframe(c->pr, idx, USART_DFRAME_SOF, NULL);
}

static void usart_add_dframe_data(struct pa_usart_ctx *ctx, uint64_t idx, uint8_t data)
{
    uint8_t *c = calloc(1, sizeof(uint8_t));
    *c = data;
    proto_add_dframe(ctx->pr, idx, USART_DFRAME_DATA, c);
}

static void usart_add_dframe_eof(struct pa_usart_ctx *c, uint64_t idx)
{
    proto_add_dframe(c->pr, idx, USART_DFRAME_EOF, NULL);
}

static void usart_add_dframe_error(struct pa_usart_ctx *c, uint64_t idx)
{
    proto_add_dframe(c->pr, idx, USART_DFRAME_ERROR, NULL);
}

proto_t *pa_usart_get_proto(struct pa_usart_ctx *c)
{
    return proto_addref(c->pr);
}

/* Reset decoder internal state
 *
 * Internally, it's cloning everything except the
 * frames and dropping the reference to the original;
 * this is done to keep any outstanding handles to
 * the frames valid.
 */
void pa_usart_reset(struct pa_usart_ctx *ctx)
{
    proto_t *new_pr;

    ctx->sample_cnt = 0;
    ctx->decode_cnt = 0;
    ctx->elapsed = (struct timespec){0};

    new_pr = proto_create();
    proto_set_period(new_pr, proto_get_period(ctx->pr));
    proto_set_note(new_pr, proto_get_note(ctx->pr));
    proto_dropref(ctx->pr);
    ctx->pr = new_pr;
}

int pa_usart_ctx_map_data(pa_usart_ctx_t *ctx, uint8_t usart_bit)
{
    if ((NULL == ctx) || (usart_bit >= MAX_SAMPLE_WIDTH))
        return -EINVAL;

    ctx->mask_usart = (1 << usart_bit);
    return 0;
}

/* Sets the sampling frequency, which is used to calculate
 * actual bit-times.  If this isn't set, the decode will
 * still work; you just can't figure out what the baud
 * rate was.
 */
int pa_usart_ctx_set_freq(pa_usart_ctx_t *ctx, float freq)
{
    if (NULL == ctx)
        return -EINVAL;

    ctx->sample_period = 1.0f / freq;
    proto_set_period(ctx->pr, ctx->sample_period);
    ctx->state->bit_width = freq * 8.68e-6;
    return 0;
}

int pa_usart_ctx_set_baud(struct pa_usart_ctx *ctx, uint64_t baud)
{
    if (NULL == ctx)
        return -EINVAL;

//    ctx->sample_period = 1.0f / baud;
    return 0;
}

/* Returns any data captured as a null-terminated string.
 * String should be free'd when no longer needed.
 */
uint64_t pa_usart_get_decoded(struct pa_usart_ctx *ctx, char **out)
{
    char *d = calloc(ctx->decode_cnt + 1, sizeof(char));
    proto_dframe_t *df = proto_dframe_first(ctx->pr);
    uint64_t bytes = 0;

    if (NULL == df) {
        *out = NULL;
        return 0;
    }

    while (bytes < ctx->decode_cnt) {
        int type = proto_dframe_type(df);
        if (USART_DFRAME_DATA == type) {
            d[bytes] = *(uint8_t *) proto_dframe_udata(df);
            bytes++;
        }
        df = proto_dframe_next(df);
    }

    *out = d;

    return ctx->decode_cnt;
}

static void fprintf_linebreak(FILE *fp, int n, const char c)
{
    for (int i = 0; i < n; i++) {
        fputc(c, fp);
    }
    fputc('\n', fp);
}

static void fprintf_center(FILE *fp, size_t n, const char *s)
{
    size_t len = strlen(s);
    unsigned midpoint = (n - len) / 2;
    for (int i = 0; i <= midpoint; i++) {
        fputs(" ", fp);
    }
    fputs(s, fp);
}

static void fprintf_indent(FILE *fp, size_t n, const char *s)
{
    for (int i = 0; i < n; i++) {
        fputs(" ", fp);
    }
    fputs(s, fp);
}


void fprint_symbols(FILE *fp, struct pa_usart_ctx *ctx, size_t wout, size_t tab)
{
    char *symbols;
    uint64_t nsymbols;
    uint64_t i = 0;

    fprintf_linebreak(fp, wout, '-');
    fprintf_center(fp, wout, "[ Symbols Received ]\n");
    nsymbols = pa_usart_get_decoded(ctx, &symbols);

    while (i < nsymbols) {
        for (int col = 0; col < wout; col++) {
            if (col < tab) {
                fputc(' ', fp);
                continue;
            } else {
                fputc(symbols[i], fp);
            }
            if (i++ == nsymbols)
                break;
        }
        fputc('\n', fp);
    }
    fprintf_linebreak(fp, wout, '-');

    free(symbols);
}


void pa_usart_fprint_report(FILE *fp, struct pa_usart_ctx *ctx)
{
    const size_t wout = 72; /* Output width */
    const size_t tab = 4; /* Tab width */
    char tmpstr[wout + 1];
    double elapsed;

    fprintf_linebreak(fp, wout, '=');
    fprintf_center(fp, wout, "-- USART Decode: 115200 @ 8-N-1 --\n");
    snprintf(tmpstr, wout, "< %s >\n", pa_usart_get_desc(ctx));
    fprintf_center(fp, wout, tmpstr);

    fprintf_linebreak(fp, wout, '=');
    snprintf(tmpstr, wout, "Samples: %lu @ %.02f MS/s\n",
        ctx->sample_cnt, 1.0E-6 / (ctx->sample_period));
    fprintf_indent(fp, tab, tmpstr);
    snprintf(tmpstr, wout, "Symbols Decoded: %lu\n", ctx->decode_cnt);
    fprintf_indent(fp, tab, tmpstr);

    elapsed = pa_usart_time_elapsed(ctx);
    snprintf(tmpstr, wout, "Total time: %.02e s\n", elapsed);
    fprintf_indent(fp, tab, tmpstr);

    snprintf(tmpstr, wout, "Average Rate: %.02e samples/s\n",
        ctx->decode_cnt/elapsed);
    fprintf_indent(fp, tab, tmpstr);
    fprint_symbols(fp, ctx, wout, tab);
}

void pa_usart_set_desc(struct pa_usart_ctx *c, const char *s)
{
    size_t len;

    if (c->desc) {
        free(c->desc);
    }

    len = strnlen(s, MAX_DESC_LEN);
    c->desc = calloc(len, sizeof(char));

    strncpy(c->desc, s, len);
}

const char *pa_usart_get_desc(struct pa_usart_ctx *c)
{
    return c->desc;
}
