/* File: pa_usart.h
 *
 * Protocol Analysis routines for Async Serial (headers)
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
#ifndef _PA_USART_H_
#define _PA_USART_H_

#include <stdint.h>

#include "cap.h"
#include "proto.h"

#ifdef __cplusplus
extern "C" {
#endif

enum pa_usart_status {
    PA_USART_IDLE,
    PA_USART_ACTIVE,
    PA_USART_DATA_VALID
};

/* Opaque type, only pa_usart functions get to touch internals. */
typedef struct pa_usart_ctx pa_usart_ctx_t;

enum usart_parity {
    USART_PARITY_NONE = 0,
    USART_PARITY_ODD = 1,
    USART_PARITY_EVEN = 2
};

enum usart_baudrates {
    USART_AUTOBAUD = 0,
    USART_BAUD_9600,
    USART_BAUD_57600,
    USART_BAUD_115200
};

enum usart_dframe_types {
    USART_DFRAME_INVALID = 0,
    USART_DFRAME_SOF,
    USART_DFRAME_DATA,
    USART_DFRAME_ERROR,
    USART_DFRAME_EOF
};

int pa_usart_ctx_init(pa_usart_ctx_t **ctx);
void pa_usart_ctx_cleanup(pa_usart_ctx_t *ctx);
int pa_usart_ctx_map_data(pa_usart_ctx_t *ctx, uint8_t usart_bit);
void pa_usart_reset(pa_usart_ctx_t *ctx);

int pa_usart_ctx_set_freq(pa_usart_ctx_t *ctx, float freq);
int pa_usart_ctx_set_symbol_length(pa_usart_ctx_t *ctx, uint8_t symbol_length);
int pa_usart_ctx_set_parity(pa_usart_ctx_t *ctx, enum usart_parity parity);
int pa_usart_ctx_set_baudrate(pa_usart_ctx_t *ctx, enum usart_baudrates baud);

/* Proto container functions */
proto_t *pa_usart_get_proto(pa_usart_ctx_t *ctx);
void pa_usart_reset_proto(pa_usart_ctx_t *ctx);

void pa_usart_decode_stream(pa_usart_ctx_t *ctx, uint32_t raw);
void pa_usart_decode_chunk(pa_usart_ctx_t *ctx, cap_t *cap);

uint64_t pa_usart_get_decoded(struct pa_usart_ctx *ctx, char **out);
void pa_usart_get_decoded_range(pa_usart_ctx_t *ctx, uint64_t start, uint64_t end, char **out);


#ifdef __cplusplus
}
#endif

#endif
