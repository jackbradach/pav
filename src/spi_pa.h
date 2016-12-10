#ifndef _SPI_PA_H_
#define _SPI_PA_H_

#include <stdint.h>

enum spi_pa_status {
    SPI_PA_IDLE,
    SPI_PA_ACTIVE,
    SPI_PA_DATA_VALID
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



typedef struct spi_pa_ctx spi_pa_ctx_t;

struct spi_pkt_buf {
    uint8_t *mosi;
    uint8_t *miso;
    uint32_t *idx;
    uint32_t len;
};

/* The SPI decoder context is meant to be opaque to the user; that way
 * the implementation can be swapped out without breaking anything
 * that's using it.
 */
int spi_pa_ctx_init(spi_pa_ctx_t **ctx);
void spi_pa_ctx_cleanup(spi_pa_ctx_t *ctx);
int spi_pa_ctx_map_mosi(spi_pa_ctx_t *ctx, uint8_t mosi_bit);
int spi_pa_ctx_map_miso(spi_pa_ctx_t *ctx, uint8_t miso_bit);
int spi_pa_ctx_map_sclk(spi_pa_ctx_t *ctx, uint8_t sclk_bit);
int spi_pa_ctx_map_cs(spi_pa_ctx_t *ctx, uint8_t cs_bit);
int spi_pa_ctx_set_symbol_length(spi_pa_ctx_t *ctx, uint8_t symbol_length);
int spi_pa_ctx_set_flags(spi_pa_ctx_t *ctx, uint8_t flag_mask);
int spi_pa_ctx_clr_flags(spi_pa_ctx_t *ctx, uint8_t flag_mask);

int spi_pa_stream(struct spi_pa_ctx *ctx, uint32_t raw_sample, uint8_t *mosi, uint8_t *miso);

#endif
