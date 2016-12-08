#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "spi_decode.h"

#define SPI_MOSI 0x1
#define SPI_MISO 0x2
#define SPI_SCLK 0x4
#define SPI_CS 0x8

#define SPI_SYMBOL_LENGTH 8
#define SAMPLE_POLARITY 1

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
 * Maps signals from a raw analyzer output (up to 64-bits wide) to a
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
static inline uint8_t unswizzle_sample(struct spi_decode_ctx *ctx, uint64_t sample)
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

/* Function: spi_decode
 *
 * Stateful decoding of a SPI stream.
 *
 * Parameters:
 *      ctx - handle to an initialized SPI Decoder context
 *      sample - a 4-bit sample, unswizzled using spi_unswizzle.
 */
int spi_decode(struct spi_decode_ctx *ctx, uint8_t sample, uint8_t *out, uint8_t *in)
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



int spi_parse(uint8_t *sbuf, uint32_t sbuf_len, struct spi_decoded *out)
{
    for (uint32_t i = 0; i < sbuf_len; i++) {

    }

    return 0;
}
