#ifndef _SPI_DECODE_H_
#define _SPI_DECODE_H_

#include <stdint.h>

enum spi_decode_status {
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



typedef struct spi_decode_ctx spi_decode_ctx_t;

struct spi_pkt_buf {
    uint8_t *mosi;
    uint8_t *miso;
    uint32_t *sample_idx;
    uint32_t len;
    uint32_t sample_rate;
};

/* The SPI decoder context is meant to be opaque to the user; that way
 * the implementation can be swapped out without breaking anything
 * that's using it.
 */
int spi_decode_ctx_init(spi_decode_ctx_t **ctx);
void spi_decode_ctx_cleanup(spi_decode_ctx_t *ctx);
int spi_decode_ctx_map_mosi(spi_decode_ctx_t *ctx, uint8_t mosi_bit);
int spi_decode_ctx_map_miso(spi_decode_ctx_t *ctx, uint8_t miso_bit);
int spi_decode_ctx_map_sclk(spi_decode_ctx_t *ctx, uint8_t sclk_bit);
int spi_decode_ctx_map_cs(spi_decode_ctx_t *ctx, uint8_t cs_bit);
int spi_decode_ctx_set_symbol_length(spi_decode_ctx_t *ctx, uint8_t symbol_length);
int spi_decode_ctx_set_flags(spi_decode_ctx_t *ctx, uint8_t flag_mask);
int spi_decode_ctx_clr_flags(spi_decode_ctx_t *ctx, uint8_t flag_mask);

int spi_decode_stream(struct spi_decode_ctx *ctx, uint32_t raw_sample, uint8_t *mosi, uint8_t *miso);

#endif
