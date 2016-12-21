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
#include <errno.h>

#include <unistd.h>

#include "pa_usart.h"

#define DEFAULT_SYMBOL_LENGTH 8
#define DEFAULT_PARITY USART_PARITY_NONE
#define DEFAULT_STOP_BITS USART_STOP_BITS_ONE

#define MAX_SAMPLE_WIDTH 32

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
    struct pa_usart_state *state;
};

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
            // TODO - jbradach - 2016/12/11 - add some sort of error feedback
            // TODO - to the caller on when a spurious bit happened.
            //printf("Spurious start bit!\n");
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
            printf("\nFraming error in STOP bit!\n");
            state->sm = USART_SM_SOF;
            state->bit_frac_cnt = 0;
        }
    } else if (state->bit_frac_cnt > (state->bit_width / 2)) {
        /* If the stop bit is truncated by a space before it's seen as
         * valid, report a framing error.  It's still a valid SOF for
         * the next frame, though.
         */
         if (USART_PA_SPACE == sample) {
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
static int stream_decoder(struct pa_usart_ctx *ctx, uint8_t sample, uint8_t *out)
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
        *out = state->data;
        state->data_valid = 0;
        state->data = 0;
        state->nbits_sampled = 0;
        return PA_USART_DATA_VALID;
    }

    return 0;
}

/* Function: pa_usart_stream
 *
 * Public interface to the SPI stream decoder; it takes a raw sample,
 * unswizzles the SPI signals that the context is registered to handle,
 * and then passes it to the stream decoder.  Caller is responsible for
 * watching the return value and collecting any bytes that are decoded.
 *
 * If the mosi or miso parameters are NULL, the decoder assumes you don't
 * care about them and will skip outputting them.  Having both NULL is
 * valid, just not particularly useful.
 *
 * Parameters:
 *      ctx - Handle to a USART decode context.
 *      raw - 32-bit raw sample from logic analyzer
 *      out - pointer to a byte where decoded data will be stored.
 *
 * Returns:
 *      pa_usart_DATA_VALID on valid data,
 *      pa_usart_IDLE / pa_usart_ACTIVE as appropriate otherwise.
 *
 */
int pa_usart_stream(struct pa_usart_ctx *ctx, uint32_t raw, uint8_t *out)
{
    uint8_t sample = unswizzle_sample(ctx, raw);
    return stream_decoder(ctx, sample, out);
}

int usart_pkt_buf_alloc(struct usart_pkt_buf **new_pbuf)
{
    struct usart_pkt_buf *pbuf;
    long page_size = sysconf(_SC_PAGESIZE);
    if (NULL == pbuf)
        return -EINVAL;

    pbuf = calloc(1, sizeof(struct usart_pkt_buf));
    pbuf->data = calloc(page_size, sizeof (uint8_t));
    pbuf->idx = calloc(page_size, sizeof (uint32_t));

    *new_pbuf = pbuf;

    return 0;
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
    if (NULL == new_ctx)
        return -EINVAL;

    /* Calloc takes care of most needed initialization, but we
     * set the symbol length to a sane default.
     */
    ctx = calloc(1, sizeof(struct pa_usart_ctx));
    ctx->symbol_length = DEFAULT_SYMBOL_LENGTH;
    ctx->state = calloc(1, sizeof(struct pa_usart_state));

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
 *      ctx - handle SPI decode context structure to destroy; this handle
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

    free(ctx);
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
    ctx->state->bit_width = freq * 8.68e-6;
    return 0;
}

int pa_usart_ctx_set_baud(pa_usart_ctx_t *ctx, uint64_t baud)
{
    if (NULL == ctx)
        return -EINVAL;

//    ctx->sample_period = 1.0f / baud;
    return 0;
}


#if 0
void spi_pkt_buf_free(struct spi_pkt_buf *pbuf)
{

}

int pa_usart_buffer(struct pa_usart_ctx *ctx, uint32_t *sbuf, uint32_t sbuf_len, struct spi_pkt_buf *out)
{
    if ((NULL == sbuf) || (NULL == out))
        return -EINVAL;

    for (unsigned long i = 0; i < (sbuf_len / sizeof(uint32_t)); i++)
    {
        uint8_t dout, din;
        int rc;

        rc = pa_usart_stream(ctx, sbuf[i], &dout, &din);
        if (pa_usart_DATA_VALID == rc) {

        }
    }

    return 0;
}





int pa_usart_ctx_map_miso(pa_usart_ctx_t *ctx, uint8_t miso_bit)
{
    if ((NULL == ctx) || (miso_bit >= MAX_SAMPLE_WIDTH))
        return -EINVAL;

    ctx->mask_miso = (1 << miso_bit);
    return 0;
}

int pa_usart_ctx_map_sclk(pa_usart_ctx_t *ctx, uint8_t sclk_bit)
{
    if ((NULL == ctx) || (sclk_bit >= MAX_SAMPLE_WIDTH))
        return -EINVAL;

    ctx->mask_sclk = (1 << sclk_bit);
    return 0;
}

int pa_usart_ctx_map_cs(pa_usart_ctx_t *ctx, uint8_t cs_bit)
{
    if ((NULL == ctx) || (cs_bit >= MAX_SAMPLE_WIDTH))
        return -EINVAL;

    ctx->mask_cs = (1 << cs_bit);
    return 0;
}




#endif
