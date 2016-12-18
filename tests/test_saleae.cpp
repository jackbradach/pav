#include <gtest/gtest.h>

#include "cap.h"
#include "saleae.h"

TEST(SaleaeTest, ImportAnalogCapture) {
    /* "Gold" values are the parameters from the uart capture */
    const char test_file[] = "bin/uart_analog_115200_50mHz.bin";
    const unsigned gold_nsamples = 116176;
    const unsigned gold_nacaps = 1;
    const unsigned gold_physical_ch = 0;
    const float gold_period = 2.0E-08;

    struct cap_analog *acap;
    struct cap_bundle *bun;

    saleae_import_analog(test_file, &bun);

    ASSERT_TRUE(NULL != bun);
    ASSERT_EQ(bun->nacaps, gold_nacaps);

    ASSERT_TRUE(NULL != bun->acaps);
    ASSERT_TRUE(NULL != bun->acaps[0]);

    acap = bun->acaps[0];
    ASSERT_EQ(acap->nsamples, gold_nsamples);
    ASSERT_EQ(acap->physical_ch, gold_physical_ch);
    ASSERT_EQ(acap->period, gold_period);
    ASSERT_TRUE(NULL != acap->samples);

}
#if 0
int main (int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
