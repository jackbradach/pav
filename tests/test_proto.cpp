#include <cstdio>
#include <gtest/gtest.h>

#include "cap.h"
#include "proto.h"
#include "queue.h"

TEST(Proto, ProtoLifeCycle) {
    const char gold_data[] = "+++Test Data Stream+++";
    proto_t *pr, *pr2;
    proto_dframe_t *df;
    unsigned refcnt;

    pr = proto_create();
    refcnt = 1;

    /* Check that structure was allocated  */
    ASSERT_TRUE(NULL != pr);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, proto_getref(pr));
    pr2 = proto_addref(pr);
    refcnt++;
    ASSERT_EQ(pr, pr2);
    ASSERT_EQ(refcnt, proto_getref(pr));

    /* Populate the proto bucket */
    for (int i = 0; i < 1000; i++) {
        proto_add_dframe(pr, i, gold_data[i % strlen(gold_data)]);
    }

    /* Verify the data */
    df = proto_dframe_first(pr);
    while (NULL != df) {
        proto_dframe_t *next = proto_dframe_next(df);;
        static int i = 0;
        uint8_t c;
        c = gold_data[i % strlen(gold_data)];
        ASSERT_EQ(i++, proto_dframe_idx(df));
        ASSERT_EQ(c, proto_dframe_data(df));
        putc(c, stdout);
        fflush(stdout);

        /* Sanity check on tail */
        if (NULL == next) {
            ASSERT_EQ(df, proto_dframe_last(pr));
        }

        df = next;
    }

    /* Teardown */
    proto_dropref(pr);
    refcnt--;
    ASSERT_EQ(refcnt, proto_getref(pr));
    proto_dropref(pr);

    /* Call with invalid pointers (should be no-op) */
    ASSERT_EQ(NULL, proto_addref(NULL));
    ASSERT_EQ(0, proto_getref(NULL));
    proto_dropref(NULL);
}
