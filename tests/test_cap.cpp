#include <gtest/gtest.h>

#include "cap.h"

TEST(CapTest, AnalogCaptureLifecycle) {
    struct cap_analog *acap;
    uint8_t refcnt = 0;

    acap = cap_analog_create();
    refcnt++;

    /* Check that structure was created */
    ASSERT_TRUE(NULL != acap);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, cap_getref((cap_base_t *) acap));
    acap = (struct cap_analog *) cap_addref((cap_base_t *) acap);
    refcnt++;
    ASSERT_EQ(refcnt, cap_getref((cap_base_t *) acap));

    /* Teardown */
    cap_dropref((cap_base_t *) acap);
    refcnt--;
    ASSERT_EQ(refcnt, cap_getref((cap_base_t *) acap));
    cap_dropref((cap_base_t *) acap);
}

#if 0
TEST(CapTest, BundleLifecycle) {
    struct cap_bundle *bun;
    uint8_t refcnt = 0;

    bun = cap_bundle_create();
    refcnt++;

    /* Check that structure was created */
    ASSERT_TRUE(NULL != bun);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, bun->rcnt.count);
    bun = cap_bundle_addref(bun);
    refcnt++;
    ASSERT_EQ(refcnt, bun->rcnt.count);

    /* Teardown */
    cap_bundle_dropref(bun);
    refcnt--;
    ASSERT_EQ(refcnt, bun->rcnt.count);
    cap_bundle_dropref(bun);
}
#endif

TEST(CapTest, DigitalCaptureLifecycle) {
    struct cap_digital *dcap;
    uint8_t refcnt = 0;

    dcap = cap_digital_create();
    refcnt++;

    /* Check that structure was created */
    ASSERT_TRUE(NULL != dcap);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, cap_getref((cap_base_t *) dcap));
    dcap = (struct cap_digital *) cap_addref((cap_base_t *) dcap);
    refcnt++;
    ASSERT_EQ(refcnt, cap_getref((cap_base_t *) dcap));

    /* Teardown */
    cap_dropref((cap_base_t *) dcap);
    refcnt--;
    ASSERT_EQ(refcnt, cap_getref((cap_base_t *) dcap));
    cap_dropref((cap_base_t *) dcap);
}



#if 0
int main (int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
