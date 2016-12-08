#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include "spi_decode.h"

#define SPI_MOSI 0x1
#define SPI_MISO 0x2
#define SPI_SCLK 0x4
#define SPI_CS 0x8

#define DEFAULT_SYMBOL_LENGTH 8
#define MAX_SAMPLE_WIDTH 32

#define SPI_FLAG_MASK (SPI_FLAG_CPOL | SPI_FLAG_CPHA | SPI_FLAG_ENDIANESS | SPI_FLAG_CS_POLARITY)

struct spi_decode_state {
    uint8_t mosi;
    uint8_t miso;
    uint8_t sclk;
    uint8_t bits_sampled;
};

/* Struct: spi_decode_ctx
 *
 * Context structure for the SPI decoder; this allows multiple
 * SPI decoders to be independently running.  They must be
 * initialized using the helper functions.
 *
 * Fields:
 *      mask_mosi - One-hot sample mask for MOSI line
 *      mask_miso - One-hot sample mask for MOSI line
 *      mask_sclk - One-hot sample mask for SCLK line
 *      mask_cs   - One-hot sample mask for CS line
 *      flags - SPI decoder configuration flags
 *      state - SPI decode state machine vars
 */
struct spi_decode_ctx {
    uint64_t mask_mosi;
    uint64_t mask_miso;
    uint64_t mask_sclk;
    uint64_t mask_cs;
    uint8_t flags;
    uint8_t symbol_length;
    struct spi_decode_state *state;
};


/* Function: is_spi_sample_point
 *
 * Helper to determine if the SPI MISO/MOSI lines need to be sampled.  It is
 * assumed that the state of the clock signal has just flipped to the
 * value passed in on sclk (which will be treated as a boolean).
 *
 * Parameters:
 *      spi_flags - SPI configuration (from context struct)
 *      sclk - Current SCLK state
 */
static inline bool is_spi_sample_point(uint8_t spi_flags, uint8_t sclk)
{
    /* CPOL = 0, CPHA = 0, Sample on sclk rising edge. */
    if ((SPI_FLAG_CPOL & ~spi_flags) &&
        (SPI_FLAG_CPHA & ~spi_flags) &&
        (!!sclk)) {
        return true;
    }

    /* CPOL = 1, CPHA = 0, Sample when sclk falling edge. */
    if ((SPI_FLAG_CPOL & spi_flags) &&
        (SPI_FLAG_CPHA & ~spi_flags) &&
        (!sclk)) {
        return true;
    }

    /* CPOL = 0, CPHA = 1, Sample on sclk rising edge. */
    if ((SPI_FLAG_CPOL & ~spi_flags) &&
        (SPI_FLAG_CPHA & spi_flags) &&
        (!!sclk)) {
        return true;
    }

    /* CPOL = 1, CPHA = 1, Sample when sclk falling edge. */
    if ((SPI_FLAG_CPOL & spi_flags) &&
        (SPI_FLAG_CPHA & spi_flags) &&
        (!sclk)) {
        return true;
    }

    /* Not time to sample yet */
    return false;
}

/* Function: do_spi_sample
 *
 * Helper to shift in MISO and MOSI data, updating the internal
 * state of the decoder.  Endianess is handled according to SPI_FLAG_ENDIANESS.
 *
 * Parameters:
 *      state - SPI decode state structure
 *      sample - SPI sample data
 */
static inline void do_spi_sample(struct spi_decode_ctx *ctx, uint8_t sample)
{
    struct spi_decode_state *state = ctx->state;
    if (ctx->flags & SPI_FLAG_ENDIANESS) {
        /* MSB First, bits will be left-shifted in. */
        state->mosi <<= 1;
        state->mosi |= (SPI_MOSI & sample) ? 1 : 0;
        state->miso <<= 1;
        state->miso |= (SPI_MISO & sample) ? 1 : 0;
    } else {
        /* LSB First, bits will be right-shifted in. */
        uint8_t shift = ctx->symbol_length - 1;
        state->mosi >>= 1;
        state->mosi |= ((SPI_MOSI & sample) ? 1 : 0) << shift;
        state->miso >>= 1;
        state->miso |= ((SPI_MISO & sample) ? 1 : 0) << shift;
    }

    state->bits_sampled++;
}

/* Function: unswizzle_sample
 *
 * Maps signals from a raw analyzer output (up to 32-bits wide) to a
 * 4-bit sample.  This is converting the actual channels to logical ones
 * for ease of processing.
 *
 * Parameters:
 *      ctx - Valid handle to SPI decode context
 *      sample - SPI sample data
 *
 * Returns:
 *      4-bit representation of SPI sample
 */
static inline uint8_t unswizzle_sample(struct spi_decode_ctx *ctx, uint32_t sample)
{
    uint8_t unswizzled = 0;

    if (sample & ctx->mask_mosi)
        unswizzled |= SPI_MOSI;

    if (sample & ctx->mask_miso)
        unswizzled |= SPI_MISO;

    if (sample & ctx->mask_sclk)
        unswizzled |= SPI_SCLK;

    if (sample & ctx->mask_cs)
        unswizzled |= SPI_CS;

    return unswizzled;
}

/* Function: stream_decoder
 *
 * Stateful decoding of a SPI stream.
 *
 * Parameters:
 *      ctx - handle to an initialized SPI Decoder context
 *      sample - a 4-bit sample, unswizzled using spi_unswizzle.
 */
static int stream_decoder(struct spi_decode_ctx *ctx, uint8_t sample, uint8_t *out, uint8_t *in)
{
    struct spi_decode_state *state = ctx->state;
    uint8_t new_sclk;

    /* Hold in IDLE if CS is inactive */
    if (sample & SPI_CS) {
        state->mosi = 0;
        state->miso = 0;
        state->sclk = 0;
        return SPI_DECODE_IDLE;
    }

    /* Detect a clock state change */
    new_sclk = !!(SPI_SCLK & sample);
    if (state->sclk != new_sclk) {
        /* Shift in a bit of MISO/MOSI if we're at a sampling point */
        if (is_spi_sample_point(ctx->flags, new_sclk)) {
            do_spi_sample(ctx, sample);
        }
        state->sclk = new_sclk;
    }

    /* Output a byte every SPI_SYMBOL_LENGTH ticks. This is a
     * "use it or lose it" situation where the caller needs to
     * be watching for SPI_DECODE_DATA_VALID as a return code.
     */
    if (state->bits_sampled == ctx->symbol_length) {
        if (NULL != out)
            *out = state->mosi;

        if (NULL != in)
            *in = state->miso;

        state->mosi = 0;
        state->miso = 0;
        state->bits_sampled = 0;
        return SPI_DECODE_DATA_VALID;
    }

    return SPI_DECODE_ACTIVE;
}

/* Function: spi_decode_stream
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
 *      ctx - Handle to a SPI decode context.
 *      raw_sample - 32-bit raw sample from logic analyzer
 *      mosi - pointer to a byte where decoded MOSI will be stored.
 *      miso - pointer to a byte where decoded MISO will be stored.
 *
 * Returns:
 *      SPI_DECODE_DATA_VALID on valid data,
 *      SPI_DECODE_IDLE / SPI_DECODE_ACTIVE as appropriate otherwise.
 *
 */
int spi_decode_stream(struct spi_decode_ctx *ctx, uint32_t raw_sample, uint8_t *mosi, uint8_t *miso)
{
    uint8_t spi_sample = unswizzle_sample(ctx, raw_sample);
    return stream_decoder(ctx, spi_sample, mosi, miso);
}

int spi_parse(uint8_t *sbuf, uint32_t sbuf_len, struct spi_decoded *out)
{
    for (uint32_t i = 0; i < sbuf_len; i++) {

    }

    return 0;
}

/* Function: spi_decode_ctx_init
 *
 * Frees the resources associated with a SPI Decode Context structure.
 * This needs to be called to avoid memory leaks when a decoder is no
 * longer needed.
 *
 * Parameters:
 *      ctx - handle SPI decode context structure to destroy; this handle
 *            will be immediately invalid after return.
 */
int spi_decode_ctx_init(struct spi_decode_ctx **new_ctx)
{
    struct spi_decode_ctx *ctx;
    struct spi_decode_state *state;
    if (NULL == new_ctx)
        return -EINVAL;

    /* Calloc takes care of most needed initialization, but we
     * set the symbol length to a sane default.
     */
    ctx = calloc(1, sizeof(struct spi_decode_ctx));
    ctx->symbol_length = DEFAULT_SYMBOL_LENGTH;
    state = calloc(1, sizeof(struct spi_decode_state));

    ctx->state = state;
    *new_ctx = ctx;
    return 0;
}

/* Function: spi_decode_cleanup_ctx
 *
 * Frees the resources associated with a SPI Decode Context structure.
 * This needs to be called to avoid memory leaks when a decoder is no
 * longer needed.
 *
 * Parameters:
 *      ctx - handle SPI decode context structure to destroy; this handle
 *            will be immediately invalid after return.
 */
void spi_decode_ctx_cleanup(struct spi_decode_ctx *ctx)
{
    if (NULL == ctx)
        return;

    if (NULL != ctx->state) {
        free(ctx->state);
        ctx->state = 0;
    }

    free(ctx);
}

int spi_decode_ctx_map_mosi(spi_decode_ctx_t *ctx, uint8_t mosi_bit)
{
    if ((NULL == ctx) || (mosi_bit >= MAX_SAMPLE_WIDTH))
        return -EINVAL;

    ctx->mask_mosi = (1 << mosi_bit);
    return 0;
}

int spi_decode_ctx_map_miso(spi_decode_ctx_t *ctx, uint8_t miso_bit)
{
    if ((NULL == ctx) || (miso_bit >= MAX_SAMPLE_WIDTH))
        return -EINVAL;

    ctx->mask_miso = (1 << miso_bit);
    return 0;
}

int spi_decode_ctx_map_sclk(spi_decode_ctx_t *ctx, uint8_t sclk_bit)
{
    if ((NULL == ctx) || (sclk_bit >= MAX_SAMPLE_WIDTH))
        return -EINVAL;

    ctx->mask_sclk = (1 << sclk_bit);
    return 0;
}

int spi_decode_ctx_map_cs(spi_decode_ctx_t *ctx, uint8_t cs_bit)
{
    if ((NULL == ctx) || (cs_bit >= MAX_SAMPLE_WIDTH))
        return -EINVAL;

    ctx->mask_cs = (1 << cs_bit);
    return 0;
}

int spi_decode_ctx_set_symbol_length(spi_decode_ctx_t *ctx, uint8_t symbol_length)
{
    if (NULL == ctx)
        return -EINVAL;

    ctx->symbol_length = symbol_length;
    return 0;
}

int spi_decode_ctx_set_flags(spi_decode_ctx_t *ctx, uint8_t set_mask)
{
    if ((NULL == ctx) || (set_mask & ~SPI_FLAG_MASK))
        return -EINVAL;

    ctx->flags |= set_mask;
    return 0;
}

int spi_decode_ctx_clr_flags(spi_decode_ctx_t *ctx, uint8_t clr_mask)
{
    if ((NULL == ctx) || (clr_mask & ~SPI_FLAG_MASK))
        return -EINVAL;

    ctx->flags &= ~clr_mask;
    return 0;
}
