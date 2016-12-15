/* File: pa_spi.h
 *
 * Protocol Analysis routines for SPI (headers)
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
#ifndef _PA_SPI_H_
#define _PA_SPI_H_

#include <stdint.h>

enum pa_spi_status {
    PA_SPI_IDLE,
    PA_SPI_ACTIVE,
    PA_SPI_DATA_VALID
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



typedef struct pa_spi_ctx pa_spi_ctx_t;

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
int pa_spi_ctx_init(pa_spi_ctx_t **ctx);
void pa_spi_ctx_cleanup(pa_spi_ctx_t *ctx);
int pa_spi_ctx_map_mosi(pa_spi_ctx_t *ctx, uint8_t mosi_bit);
int pa_spi_ctx_map_miso(pa_spi_ctx_t *ctx, uint8_t miso_bit);
int pa_spi_ctx_map_sclk(pa_spi_ctx_t *ctx, uint8_t sclk_bit);
int pa_spi_ctx_map_cs(pa_spi_ctx_t *ctx, uint8_t cs_bit);
int pa_spi_ctx_set_symbol_length(pa_spi_ctx_t *ctx, uint8_t symbol_length);
int pa_spi_ctx_set_flags(pa_spi_ctx_t *ctx, uint8_t flag_mask);
int pa_spi_ctx_clr_flags(pa_spi_ctx_t *ctx, uint8_t flag_mask);

int pa_spi_stream(struct pa_spi_ctx *ctx, uint32_t raw_sample, uint8_t *mosi, uint8_t *miso);

#endif
