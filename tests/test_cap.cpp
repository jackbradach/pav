#include <gtest/gtest.h>

#include "cap.h"

TEST(CapTest, AnalogCaptureLifecycle) {
    struct cap_analog *cap;
    uint8_t refcnt = 0;

    cap = cap_analog_create(NULL);
    refcnt++;

    /* Check that structure was created */
    ASSERT_TRUE(NULL != cap);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, cap->rcnt.count);
    cap = cap_analog_create(cap);
    refcnt++;
    ASSERT_EQ(refcnt, cap->rcnt.count);

    /* Teardown */
    cap_analog_destroy(cap);
    refcnt--;
    ASSERT_EQ(refcnt, cap->rcnt.count);
    cap_analog_destroy(cap);
}

TEST(CapTest, BundleLifecycle) {
    struct cap_bundle *bun;
    uint8_t refcnt = 0;

    bun = cap_bundle_create(NULL);
    refcnt++;

    /* Check that structure was created */
    ASSERT_TRUE(NULL != bun);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, bun->rcnt.count);
    bun = cap_bundle_create(bun);
    refcnt++;
    ASSERT_EQ(refcnt, bun->rcnt.count);

    /* Teardown */
    cap_bundle_destroy(bun);
    refcnt--;
    ASSERT_EQ(refcnt, bun->rcnt.count);
    cap_bundle_destroy(bun);
}

TEST(CapTest, DigitalCaptureLifecycle) {
    struct cap_digital *cap;
    uint8_t refcnt = 0;

    cap = cap_digital_create(NULL);
    refcnt++;

    /* Check that structure was created */
    ASSERT_TRUE(NULL != cap);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, cap->rcnt.count);
    cap = cap_digital_create(cap);
    refcnt++;
    ASSERT_EQ(refcnt, cap->rcnt.count);

    /* Teardown */
    cap_digital_destroy(cap);
    refcnt--;
    ASSERT_EQ(refcnt, cap->rcnt.count);
    cap_digital_destroy(cap);
}



#if 0
int main (int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
