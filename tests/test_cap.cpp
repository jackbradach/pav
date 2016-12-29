#include <gtest/gtest.h>

#include "cap.h"

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
