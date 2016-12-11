#ifndef _PA_USART_H_
#define _PA_USART_H_

#include <stdint.h>

enum pa_usart_status {
    PA_USART_IDLE,
    PA_USART_ACTIVE,
    PA_USART_DATA_VALID
};


/* Opaque type, only pa_usart functions get to touch internals. */
typedef struct pa_usart_ctx pa_usart_ctx_t;

struct usart_pkt_buf {
    uint8_t *data;
    uint32_t *idx;
    uint32_t len;
};

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

int pa_usart_ctx_init(pa_usart_ctx_t **ctx);
void pa_usart_ctx_cleanup(pa_usart_ctx_t *ctx);
int pa_usart_ctx_map_data(pa_usart_ctx_t *ctx, uint8_t usart_bit);

int pa_usart_ctx_set_freq(pa_usart_ctx_t *ctx, float freq);
int pa_usart_ctx_set_symbol_length(pa_usart_ctx_t *ctx, uint8_t symbol_length);
int pa_usart_ctx_set_parity(pa_usart_ctx_t *ctx, enum usart_parity parity);
int pa_usart_ctx_set_baudrate(pa_usart_ctx_t *ctx, enum usart_baudrates baud);

int pa_usart_stream(struct pa_usart_ctx *ctx, uint32_t raw_sample, uint8_t *out);

#endif
