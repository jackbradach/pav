#include <gtest/gtest.h>

#include "cap.h"

TEST(CapTest, AnalogCaptureLifecycle) {
    cap_analog_t *acap;
    uint8_t refcnt = 0;
    unsigned nsamples = 1000;

    acap = cap_analog_create(nsamples);
    refcnt++;

    /* Check that structure was created */
    ASSERT_TRUE(NULL != acap);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, cap_getref((cap_t *) acap));
    acap = (struct cap_analog *) cap_addref((cap_t *) acap);
    refcnt++;
    ASSERT_EQ(refcnt, cap_getref((cap_t *) acap));

    /* Teardown */
    cap_dropref((cap_t *) acap);
    refcnt--;
    ASSERT_EQ(refcnt, cap_getref((cap_t *) acap));
    cap_dropref((cap_t *) acap);

    /* Call with invalid pointers (should be no-op) */
    ASSERT_EQ(NULL, cap_addref(NULL));
    cap_dropref(NULL);
    ASSERT_EQ(0, cap_getref(NULL));
}

TEST(CapTest, BundleLifecycle) {
    cap_bundle_t *bun;
    uint8_t refcnt;

    bun = cap_bundle_create();
    refcnt = 1;

    /* Check that structure was created */
    ASSERT_TRUE(NULL != bun);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, cap_bundle_getref(bun));
    bun = cap_bundle_addref(bun);
    refcnt++;
    ASSERT_EQ(refcnt, cap_bundle_getref(bun));

    /* Teardown */
    cap_bundle_dropref(bun);
    refcnt--;
    ASSERT_EQ(refcnt, cap_bundle_getref(bun));
    cap_bundle_dropref(bun);

    /* Call with invalid pointers (should be no-op) */
    ASSERT_EQ(NULL, cap_bundle_addref(NULL));
    cap_bundle_dropref(NULL);
    ASSERT_EQ(0, cap_bundle_getref(NULL));

}

TEST(CapTest, DigitalCaptureLifecycle) {
    cap_digital_t *dcap;
    unsigned nsamples = 1000;
    uint8_t refcnt = 0;

    dcap = cap_digital_create(nsamples);
    refcnt++;

    /* Check that structure was created */
    ASSERT_TRUE(NULL != dcap);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, cap_getref((cap_t *) dcap));
    dcap = (struct cap_digital *) cap_addref((cap_t *) dcap);
    refcnt++;
    ASSERT_EQ(refcnt, cap_getref((cap_t *) dcap));

    /* Teardown */
    cap_dropref((cap_t *) dcap);
    refcnt--;
    ASSERT_EQ(refcnt, cap_getref((cap_t *) dcap));
    cap_dropref((cap_t *) dcap);
}
