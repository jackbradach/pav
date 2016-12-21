#include <gtest/gtest.h>

#include "cap.h"
#include "pa_usart.h"
#include "saleae.h"
#include "queue.h"

#define SAMPLE_DIR "bin/"

TEST(PaUsartTest, FunctionalDigital) {
    pa_usart_ctx_t *usart_ctx;

    uint64_t sample_count = 0;
    uint64_t decode_count = 0;
    struct cap_digital *dcap;
    int rc;

    /* Init USART decode, all defaults are fine. */
    pa_usart_ctx_init(&usart_ctx);
    pa_usart_ctx_map_data(usart_ctx, 0);

    /* Map sample file into memory */
    // XXX - need to convert these to test fixtures!
    rc = saleae_import_digital(SAMPLE_DIR "uart_digital_115200_500mHz.bin", sizeof(uint32_t), 500.0E6, &dcap);
    ASSERT_EQ(rc, 0);

    pa_usart_ctx_set_freq(usart_ctx, 500.0E6);

    for (unsigned long i = 0; i < dcap->super.nsamples; i++)
    {
        uint8_t dout;
        int rc;
        rc = pa_usart_stream(usart_ctx, dcap->samples[i], &dout);
        if (PA_USART_DATA_VALID == rc) {
            decode_count++;
        }
        sample_count++;
    }
    pa_usart_ctx_cleanup(usart_ctx);
}

TEST(PaUsartTest, FunctionalAnalog) {
    pa_usart_ctx_t *usart_ctx;

    uint64_t sample_count = 0;
    uint64_t decode_count = 0;
    const uint64_t gold_decode_count = 22;
    struct cap_bundle *bun;
    struct cap_analog *acap;

    /* Init USART decode, all defaults are fine. */
    pa_usart_ctx_init(&usart_ctx);
    pa_usart_ctx_map_data(usart_ctx, 0);

    /* Map sample file into memory */
    // XXX - need to convert these to test fixtures!
    saleae_import_analog(SAMPLE_DIR "uart_analog_115200_50mHz.bin", &bun);

    acap = (struct cap_analog *) TAILQ_FIRST(bun->caps);

    pa_usart_ctx_set_freq(usart_ctx, 50.0E6);

    for (unsigned long i = 0; i < acap->super.nsamples; i++)
    {
        uint8_t dout;
        int rc;
        rc = pa_usart_stream(usart_ctx, acap->dcap->samples[i], &dout);
        if (PA_USART_DATA_VALID == rc) {
            decode_count++;
        }
        sample_count++;
    }
    ASSERT_EQ(decode_count, gold_decode_count);
    pa_usart_ctx_cleanup(usart_ctx);
}
