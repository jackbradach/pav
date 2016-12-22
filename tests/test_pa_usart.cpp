#include <gtest/gtest.h>
#include "test_utils.hpp"

#include "cap.h"
#include "pa_usart.h"
#include "saleae.h"

TEST(PaUsartTest, DISABLED_FunctionalDigital) {
    pa_usart_ctx_t *usart_ctx;
    cap_digital_t *dcap;
    const char test_file[] = "uart_digital_115200_500mHz.bin.gz";
    FILE *fp = fopen(test_file, "rb");
    int rc;

    /* Init USART decode, all defaults are fine. */
    pa_usart_ctx_init(&usart_ctx);
    pa_usart_ctx_map_data(usart_ctx, 0);

    /* Map sample file into memory */
    // XXX - need to convert these to test fixtures!
    rc = saleae_import_digital(fp, sizeof(uint32_t), 500.0E6, &dcap);
    ASSERT_EQ(rc, 0);

    pa_usart_ctx_set_freq(usart_ctx, 500.0E6);

    for (unsigned long i = 0; i < cap_get_nsamples((cap_t *) dcap); i++)
    {
        uint32_t sample;
        sample = cap_digital_get_sample(dcap, i);
        pa_usart_decode_stream(usart_ctx, sample);
    }
    pa_usart_ctx_cleanup(usart_ctx);
}

TEST(PaUsartTest, UsartStream) {
    FILE *fp = fopen("uart_analog_115200_50mHz.bin.gz", "rb");
    pa_usart_ctx_t *usart_ctx;
    cap_bundle_t *bun;
    cap_t *cap;
    cap_digital_t *dcap;

    /* Init USART decode, all defaults are fine. */
    pa_usart_ctx_init(&usart_ctx);
    pa_usart_ctx_map_data(usart_ctx, 0);

    saleae_import_analog(fp, &bun);
    pa_usart_ctx_set_freq(usart_ctx, 50.0E6);

    cap = cap_bundle_first(bun);
    dcap = cap_get_digital(cap);

    for (unsigned long i = 0; i < cap_get_nsamples(cap); i++)
    {
        pa_usart_decode_stream(usart_ctx, cap_digital_get_sample(dcap, i));
    }

    pa_usart_ctx_cleanup(usart_ctx);
    cap_dropref((cap_t *) dcap);
    cap_dropref(cap);
}

TEST(PaUsartTest, UsartBlock) {
    TEST_DESC("Tests the USART decoder on a block of samples");
    const char gold_usart_recv[] = "Uart Decode Test PASS!";
    size_t gold_decode_cnt = strlen(gold_usart_recv);
    FILE *fp = fopen("uart_analog_115200_50mHz.bin.gz", "rb");
    char *usart_recv;
    uint64_t decode_cnt;

    pa_usart_ctx_t *usart;
    cap_bundle_t *bun;
    cap_t *cap;

    /* Init USART decode, all defaults are fine. */
    pa_usart_ctx_init(&usart);
    pa_usart_ctx_map_data(usart, 0);

    saleae_import_analog(fp, &bun);
    pa_usart_ctx_set_freq(usart, 50.0E6);

    cap = cap_bundle_first(bun);
    pa_usart_decode_chunk(usart, cap);

    decode_cnt = pa_usart_get_decoded(usart, &usart_recv);
    ASSERT_EQ(decode_cnt, gold_decode_cnt);
    ASSERT_STREQ(usart_recv, gold_usart_recv);

    free(usart_recv);
    pa_usart_ctx_cleanup(usart);
    pa_usart_reset(usart);
    fclose(fp);
}
