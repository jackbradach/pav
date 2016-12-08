#ifndef _SPI_DECODE_H_
#define _SPI_DECODE_H_

#include <stdint.h>

enum spi_decode_states {
    SPI_DECODE_IDLE,
    SPI_DECODE_ACTIVE,
    SPI_DECODE_DATA_VALID
};

/* Enum: spi_flags
 *
 * SPI_FLAG_CPOL - Clock polarity when inactive
 * SPI_FLAG_CPHA - Data valid on clock rising (CPHA=0) or falling (CPHA=1) edge
 * SPI_FLAG_ENDIANESS - Data endianess; 1 for MSB first, 0 for LSB first
 * SPI_FLAG_CS_POLARITY - Polarity of enable line when active
 */
enum spi_flags {
    SPI_FLAG_CPOL = 0x1,
    SPI_FLAG_CPHA = 0x2,
    SPI_FLAG_ENDIANESS = 0x4,
    SPI_FLAG_CS_POLARITY = 0x8
};

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

typedef struct spi_decode_ctx spi_decode_ctx_t;

struct spi_decoded {
    uint8_t *mosi;
    uint8_t *miso;
    uint32_t *sample_idx;
    uint32_t len;

    uint32_t sample_rate;
};


int spi_decode_init_ctx(spi_decode_ctx_t **ctx);
void spi_decode_cleanup_ctx(spi_decode_ctx_t *ctx);

int spi_decode(spi_decode_ctx_t *ctx, uint8_t sample, uint8_t *out, uint8_t *in);

#endif
