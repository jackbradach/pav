#include <cerrno>
#include <gtest/gtest.h>


#include "cap.h"
#include "saleae.h"

extern char SAMPLE_PATH[];

TEST(SaleaeTest, ImportAnalogCapture) {
    /* "Gold" values are the parameters from the uart capture */
    const char test_file[] = "uart_analog_115200_50mHz.bin.gz";
    const unsigned gold_nsamples = 116176;
    const unsigned gold_ncaps = 1;
    const unsigned gold_physical_ch = 0;
    const float gold_period = 2.0E-08;

    cap_bundle_t *bun;
    cap_t *cur;

    char path[512] = {0};
    if (strlen(SAMPLE_PATH) > 0)
        strcat(path, SAMPLE_PATH);
    strcat(path, test_file);

    unsigned cap_count = 0;
    FILE *fp = fopen(path, "rb");
    saleae_import_analog(fp, &bun);

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
    fclose(fp);
}

TEST(SaleaeTest, ImportAnalogBogusInput) {
    cap_bundle_t *bun = (cap_bundle_t *) 0xf00fb00b;
    int rc;

    rc = saleae_import_analog(NULL, &bun);
    ASSERT_TRUE(rc < 0);
    ASSERT_EQ(errno, EIO);

}
