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


}


TEST(CapTest, DigitalCaptureLifecycle) {


}


int main (int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
