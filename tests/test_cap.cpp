#include <gtest/gtest.h>

#include "cap.h"
#include "saleae.h"

TEST(CapTest, CaptureLifecycle) {
    cap_t *c;
    uint8_t refcnt = 0;
    unsigned nsamples = 1000;

    c = cap_create(nsamples);
    refcnt++;

    /* Check that structure was created */
    ASSERT_TRUE(NULL != c);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, cap_nref(c));
    c = cap_addref(c);
    refcnt++;
    ASSERT_EQ(refcnt, cap_nref(c));

    /* Teardown */
    cap_dropref(c);
    refcnt--;
    ASSERT_EQ(refcnt, cap_nref(c));
    cap_dropref(c);

    /* Call with invalid pointers (should be no-op) */
    ASSERT_EQ(NULL, cap_addref(NULL));
    cap_dropref(NULL);
    ASSERT_EQ(0, cap_nref(NULL));
}

TEST(CapTest, BundleLifecycle) {
    cap_bundle_t *bun;
    uint8_t refcnt;

    bun = cap_bundle_create();
    refcnt = 1;

    /* Check that structure was created */
    ASSERT_TRUE(NULL != bun);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, cap_bundle_nref(bun));
    bun = cap_bundle_addref(bun);
    refcnt++;
    ASSERT_EQ(refcnt, cap_bundle_nref(bun));

    /* Teardown */
    cap_bundle_dropref(bun);
    refcnt--;
    ASSERT_EQ(refcnt, cap_bundle_nref(bun));
    cap_bundle_dropref(bun);

    /* Call with invalid pointers (should be no-op) */
    ASSERT_EQ(NULL, cap_bundle_addref(NULL));
    cap_bundle_dropref(NULL);
    ASSERT_EQ(0, cap_bundle_nref(NULL));
}

TEST(CapTest, CloneToBundle) {
    /* "Gold" values are the parameters from the uart capture */
    const char test_file[] = "uart_analog_115200_50mHz.bin.gz";
    const unsigned gold_nsamples = 116176;
    const unsigned gold_ncaps = 1;
    const unsigned gold_physical_ch = 0;
    const float gold_period = 2.0E-08;

    cap_bundle_t *bun_src, *b1;
    cap_t *cap_src, *c1;

    FILE *fp = fopen(test_file, "rb");
    saleae_import_analog(fp, &bun_src);
    cap_src = cap_bundle_first(bun_src);

    b1 = cap_bundle_create();
    ASSERT_EQ(cap_bundle_len(b1), 0);
    ASSERT_TRUE(NULL == cap_bundle_first(b1));
    cap_clone_to_bundle(b1, cap_src, 1, 0);
    c1 = cap_bundle_first(b1);
    ASSERT_EQ(cap_bundle_len(b1), 1);
    ASSERT_TRUE(NULL != c1);
    ASSERT_EQ(cap_get_period(c1), cap_get_period(cap_src));
    ASSERT_EQ(cap_get_analog_min(c1), cap_get_analog_min(cap_src));
    ASSERT_EQ(cap_get_analog_max(c1), cap_get_analog_max(cap_src));

    ASSERT_TRUE(cap_get_analog_cal(c1) == cap_get_analog_cal(cap_src));

}
