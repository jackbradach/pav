#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "refcnt.h"
#include "queue.h"

#include "proto.h"


/* Struct: proto_dframe
 *
 * A decoded data frame list entry.
 *
 * Elements:
 *  idx - sample index from protocol analyzer.
 *  frame - data byte
 *
 */
struct proto_dframe {
    TAILQ_ENTRY(proto_dframe) entry;
    uint64_t idx;
    int type;
    void *udata;
};
TAILQ_HEAD(dframes_list, proto_dframe);

/* Struct: proto
 *
 * Container for holding data frames that a protocol analyzer decodes.
 *
 */
struct proto {
    char note[PROTO_MAX_NOTE_LEN];
    struct refcnt rcnt;
    float period;
    uint64_t nframes;
    struct dframes_list head;
};

void proto_add_dframe(struct proto *pr, uint64_t idx, int type, void *udata)
{
    struct proto_dframe *df;

    df = calloc(1, sizeof(struct proto_dframe));
    df->idx = idx;
    df->type = type;
    df->udata = udata;
    TAILQ_INSERT_TAIL(&pr->head, df, entry);
    pr->nframes++;
}


static void proto_free(const struct refcnt *ref);


/* Function: proto_create
 *
 * Allocates and initializes a new proto structure
 *
 */
struct proto *proto_create(void)
{
    struct proto *pr;

    pr = calloc(1, sizeof(struct proto));
    pr->rcnt = (struct refcnt) { proto_free, 1 };
    TAILQ_INIT(&pr->head);
    return pr;
}

/* Function: proto_addref
 *
 * Adds a reference to a proto_t, returning a pointer to the caller.
 *
 * Parameters:
 *  pr - existing proto_t structure
 *
 * Returns:
 *  pointer to capture structure for caller
 *
 * See Also:
 *  <proto_dropref>
 */
struct proto *proto_addref(struct proto *pr)
{
    if (NULL == pr)
        return NULL;

    refcnt_inc(&pr->rcnt);
    return pr;
}

/* Function: proto_dropref
 *
 * Mark an proto data structure as no longer used; if no otherwise
 * references are held to the structure, it'll be cleaned up and the
 * memory freed.
 *
 * Parameters:
 *  p - handle to an proto structure
 */
void proto_dropref(struct proto *p)
{
    if (NULL == p)
        return;
    refcnt_dec(&p->rcnt);
}

/* Function: proto_getref
 *
 * Returns the number of active references on a proto_t
 *
 * Parameters:
 *  p - pointer to a proto_t
 *
 * Returns:
 *  Number of active references
 *
 * See Also:
 *  <proto_addref>, <proto_dropref>
 */
unsigned proto_getref(struct proto *p)
{
    if (NULL == p)
        return 0;
    return p->rcnt.count;
}

static void proto_free(const struct refcnt *ref)
{
    proto_dframe_t *cur, *tmp;
    struct proto *pr =
        container_of(ref, struct proto, rcnt);

    TAILQ_FOREACH_SAFE(cur, &pr->head, entry, tmp) {
        TAILQ_REMOVE(&pr->head, cur, entry);
        if (cur->udata)
            free(cur->udata);
        free(cur);
    }

    free(pr);
}

void *proto_dframe_udata(struct proto_dframe *df)
{
    return df->udata;
}

uint64_t proto_dframe_idx(struct proto_dframe *df)
{
    return df->idx;
}

int proto_dframe_type(struct proto_dframe *df)
{
    return df->type;
}

struct proto_dframe *proto_dframe_first(struct proto *pr)
{
    return TAILQ_FIRST(&pr->head);
}

struct proto_dframe *proto_dframe_next(struct proto_dframe *df)
{
    return TAILQ_NEXT(df, entry);
}

struct proto_dframe *proto_dframe_last(struct proto *pr)
{
    return TAILQ_LAST(&pr->head, dframes_list);
}

void proto_set_note(proto_t *pr, const char *s)
{
    strncpy(pr->note, s, PROTO_MAX_NOTE_LEN);
}

const char *proto_get_note(struct proto *pr)
{
    return &pr->note;
}

uint64_t proto_get_nframes(struct proto *pr)
{
    return pr->nframes;
}
