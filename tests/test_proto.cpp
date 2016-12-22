#include <cstdio>
#include <gtest/gtest.h>
#include "test_utils.hpp"

#include "cap.h"
#include "proto.h"
#include "queue.h"

TEST(Proto, ProtoLifeCycle) {
    const char gold_data[] = "+++Test Data Stream+++";
    const char gold_note[] = "---Test Note---";
    float gold_period = 1.2345678E5;
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
    proto_set_note(pr, gold_note);
    ASSERT_STREQ(gold_note, proto_get_note(pr));
    proto_set_period(pr, gold_period);
    ASSERT_FLOAT_EQ(gold_period, proto_get_period(pr));


    for (int i = 0; i < 1000; i++) {
        uint8_t *c = (uint8_t *) malloc(sizeof(uint8_t));
        *c = gold_data[i % strlen(gold_data)];
        proto_add_dframe(pr, i, ~i, (void *) c);
    }

    ASSERT_EQ(1000, proto_get_nframes(pr));

    /* Verify the data */
    df = proto_dframe_first(pr);
    while (NULL != df) {
        proto_dframe_t *next = proto_dframe_next(df);;
        static int i = 0;
        uint8_t c = gold_data[i % strlen(gold_data)];
        ASSERT_EQ(i, proto_dframe_idx(df));
        ASSERT_EQ(c, *(uint8_t *) proto_dframe_udata(df));
        ASSERT_EQ(~i, proto_dframe_type(df));

        /* Sanity check on tail */
        if (NULL == next) {
            ASSERT_EQ(df, proto_dframe_last(pr));
        }
        i++;
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

    /* Setup/Teardown with no data payload */
    pr = proto_create();
    proto_add_dframe(pr, 0, 0, NULL);
    proto_dropref(pr);
}
