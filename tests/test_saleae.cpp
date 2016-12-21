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

    struct cap_bundle *bun;
    cap_t *cur;
    unsigned cap_count = 0;

    saleae_import_analog(test_file, &bun);

    ASSERT_TRUE(NULL != bun);


    //ASSERT_TRUE(NULL != bun->acaps);
    //ASSERT_TRUE(NULL != bun->acaps[0]);

    TAILQ_FOREACH(cur, bun->caps, entry) {
        struct cap_analog *acap = (struct cap_analog *) cur;
        ASSERT_EQ(cur->nsamples, gold_nsamples);
        ASSERT_EQ(cur->physical_ch, gold_physical_ch);
        ASSERT_EQ(cur->period, gold_period);
        ASSERT_TRUE(NULL != acap->samples);
        cap_count++;
    }
    ASSERT_EQ(cap_count, gold_ncaps);

//    cap_bundle_destroy(bun);
}

#if 0
int main (int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
