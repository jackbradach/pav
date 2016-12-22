#include <gtest/gtest.h>

#include "cap.h"
#include "saleae.h"
#include "queue.h"

#define SAMPLE_DIR "bin/"

TEST(SaleaeTest, ImportAnalogCapture) {
    /* "Gold" values are the parameters from the uart capture */
    const char test_file[] = SAMPLE_DIR "uart_analog_115200_50mHz.bin";
    const unsigned gold_nsamples = 116176;
    const unsigned gold_ncaps = 1;
    const unsigned gold_physical_ch = 0;
    const float gold_period = 2.0E-08;

    cap_bundle_t *bun;
    cap_t *cur;
    unsigned cap_count = 0;

    saleae_import_analog(test_file, &bun);

    ASSERT_TRUE(NULL != bun);

    cur = cap_bundle_first(bun);
    while (cur) {
        ASSERT_EQ(cap_get_nsamples(cur), gold_nsamples);
        ASSERT_EQ(cap_get_physical_ch(cur), gold_physical_ch);
        ASSERT_EQ(cap_get_period(cur), gold_period);
        cap_count++;
        cur = cap_next(cur);
    }
    ASSERT_EQ(cap_count, gold_ncaps);

    cap_bundle_dropref(bun);
}

#if 0
int main (int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
