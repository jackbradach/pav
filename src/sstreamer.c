/* Sample Streamer, responsible for directing streams towards
 * protocol analyzers.
 */

#include <assert.h>
#include <string.h>

#include "adc.h"
#include "sstreamer.h"



void sstreamer_ctx_alloc(struct sstreamer_ctx **uctx)
{
    struct sstreamer_ctx *ctx;

    // TODO - runtime error handling
    assert(NULL != uctx);

    ctx = calloc(2, sizeof(struct sstreamer_ctx));
    /* Subscriber structure pointers.  Allocate enough space for one
     * up front; we'll realloc geometrically as needed.
     */
    ctx->subs = calloc(1, sizeof(struct sstreamer_sub *));
    ctx->maxsubs = 1;

    *uctx = ctx;
}

void sstreamer_ctx_free(struct sstreamer_ctx *ctx)
{
    if (NULL == ctx)
        return;

    if (ctx->nsubs) {
        for (int idx = 0; idx < ctx->nsubs; idx++) {
            struct sstreamer_sub *sub = ctx->subs[idx];
            ctx->subs[idx] = 0;
            free(sub);
        }
    }
}

void sstreamer_sub_alloc(struct sstreamer_sub **usub, void *ctx)
{
    struct sstreamer_sub *sub;
    // TODO - runtime error handling
    assert(NULL != usub);

    sub = calloc(1, sizeof(struct sstreamer_sub));
    sub->ctx = ctx;
    *usub = sub;
}

void sstreamer_sub_free(struct sstreamer_sub *sub)
{
    if (NULL != sub)
        free(sub);
}


void sstreamer_add_sub(struct sstreamer_ctx *ctx, struct sstreamer_sub *sub)
{
    /* Reallocate the subscribers array if it's at capacity. */
    if (ctx->nsubs == ctx->maxsubs) {
        struct sstreamer_sub **new_subs;

        new_subs = realloc(ctx->subs, 2 * ctx->maxsubs * sizeof(struct sstreamer_sub **));
        // TODO - add graceful handling although this should never happen.
        assert(NULL != new_subs);
        ctx->subs = new_subs;
        ctx->maxsubs *= 2;
    }

    ctx->subs[ctx->nsubs] = sub;
    ctx->nsubs++;
}
