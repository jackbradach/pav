#include "cap.h"
#include "gui.h"
#include "views.h"

struct view {
    TAILQ_ENTRY(view) entry;
    cap_t *cap;
    plot_t *pl;
    SDL_Texture *txt;
    shader_t *shader;
    uint64_t begin;
    uint64_t end;
    int64_t target;
    unsigned zoom;
    uint32_t flags;
};
TAILQ_HEAD(view_list, view);

struct views {
    struct view_list head;
    unsigned len;
};

void views_add_ch(struct views *vl, cap_t *c);
struct view *view_from_ch(cap_t *c);

void views_create_list(struct views **vl)
{
    struct views *views;

    views = calloc(1, sizeof(struct views));
    TAILQ_INIT(&views->head);
    *vl = views;
}

cap_t *views_get_cap(view_t *v)
{
    return v->cap;
}

SDL_Texture *views_get_texture(view_t *v)
{
    return v->txt;
}

unsigned views_get_zoom(view_t *v)
{
    return v->zoom;
}

view_t *views_first(views_t *vl)
{
    return TAILQ_FIRST(&vl->head);
}

view_t *views_next(view_t *v)
{
    return TAILQ_NEXT(v, entry);
}

view_t *views_prev(view_t *v)
{
    return TAILQ_PREV(v, view_list, entry);
}

view_t *views_last(views_t *vl)
{
    return TAILQ_LAST(&vl->head, view_list);
}


unsigned long views_get_begin(view_t *v)
{
    return v->begin;
}

unsigned long views_get_end(view_t *v)
{
    return v->begin;
}

unsigned long views_get_width(view_t *v)
{
    return(v->end - v->begin);
}

void views_set_range(view_t *v, int64_t begin, int64_t end)
{
    cap_t *c = views_get_cap(v);
    uint64_t nsamples = cap_get_nsamples(c);

    if (begin > 0) {
        v->begin = begin;
    } else {
        v->begin = 0;
    }

    if ((end > 0) && (end < nsamples)) {
        v->end = end;
    } else {
        v->end = nsamples;
    }
}

int64_t views_get_target(view_t *v)
{
    return v->target;
}

void views_set_target(view_t *v, int64_t n)
{
    v->target = n;
}

void views_populate_from_bundle(cap_bundle_t *b, struct views **vl)
{
    struct gui *g = gui_get_instance();
    unsigned len = cap_bundle_len(b);
    struct views *views;
    cap_t *c;

    views_create_list(&views);

    c = cap_bundle_first(b);
    while (NULL != c) {
        struct view *v = view_from_ch(c);
        if (v->txt) {
            SDL_DestroyTexture(v->txt);
        }
        v->txt= SDL_CreateTexture(g->renderer,
                    SDL_PIXELFORMAT_ARGB8888,
                    SDL_TEXTUREACCESS_STREAMING,
                    GUI_WIDTH, GUI_HEIGHT / len);
        TAILQ_INSERT_TAIL(&views->head, v, entry);
        c = cap_next(c);
    }

    *vl = views;
}

struct view *view_from_ch(cap_t *c)
{
    struct view *v;

    v = calloc(1, sizeof(struct view));
    v->cap = cap_addref(c);
    v->target = cap_get_nsamples(c) / 2;
    v->begin = 0;
    v->end = cap_get_nsamples(c);
    v->zoom = 1;
    v->flags |= VIEW_PLOT_DIRTY;
    return v;
}

void views_destroy(struct views *vl)
{
    struct view *cur, *tmp;

    TAILQ_FOREACH_SAFE(cur, &vl->head, entry, tmp) {
        TAILQ_REMOVE(&vl->head, cur, entry);
        cap_dropref(cur->cap);
        plot_dropref(cur->pl);
        free(cur);
    }
    free(vl);
}

void views_refresh(struct view *v)
{
    /* Refresh the plot if needed (eg, zoom / pan) */
    if (v->flags & VIEW_PLOT_DIRTY) {
        plot_from_view(v, &v->pl);
        v->flags &= ~VIEW_PLOT_DIRTY;
        v->flags |= VIEW_TEXTURE_DIRTY;
    }

    /* Refesh the texture if dirty (eg, the size changed) */
    if (v->flags & VIEW_TEXTURE_DIRTY) {
        plot_to_texture(v->pl, v->txt);
        v->flags &= ~VIEW_TEXTURE_DIRTY;
    }
}

void views_zoom_in(struct view *v)
{
    int64_t z0, z1;
    uint64_t nsamples;
    uint64_t znsamples;

    nsamples = cap_get_nsamples(v->cap);

    znsamples = nsamples / (v->zoom + 1);
    z0 = v->target - (znsamples / 2);
    z1 = v->target + (znsamples / 2);

    printf("znsamples: %'lu z0: %'ld z1: %'ld target: %'ld\n",
        znsamples, z0, z1, v->target);

    /* Handle zooming at edges; it'll redistribute to the left or
     * right as needed.
     */
    if (z0 < 0) {
        z1 += -z0;
        z0 += z0;
    }

    if (z1 >= nsamples) {
        z0 -= (z1 - nsamples);
        z1 -= (z1 - nsamples);
    }

    if (z0 < 0) {
        printf("MAXIMUM ZOOM\n");
        return;
    }

    // TODO - critical section
    v->begin = z0;
    v->end = z1;
    v->zoom++;
    v->flags |= VIEW_PLOT_DIRTY;
}

void views_zoom_out(struct view *v)
{
    cap_t *top;
    int64_t z0, z1, zoom_gran;

    top = cap_get_top(v->cap);

    /* If there's no parent, this is the top-level zoom! */
    if (top == v->cap) {
        printf("Unable to zoom out further (Top)!\n");
        return;
    }

    zoom_gran = cap_get_nsamples(v->cap) / 2;
    z0 = cap_get_offset(v->cap) - zoom_gran;
    z1 = cap_get_offset(v->cap) + cap_get_nsamples(v->cap) + zoom_gran;

    if (z0 < 0) {
        z0 = 0;
    }

    if (z1 > (cap_get_nsamples(top) - 1)) {
        z1 = cap_get_nsamples(top) - 1;
    }

    // TODO - critical section
    if ((z0 == 0) && (z1 == (cap_get_nsamples(top) - 1))) {
        v->cap = top;
    } else {
        cap_t *old = v->cap;
        v->cap = cap_create_subcap(top, z0, z1);
        if (old != top)
            cap_dropref(old);
    }
    v->flags |= VIEW_PLOT_DIRTY;
}

void views_pan_left(struct view *v)
{
    cap_t *top;
    int64_t mid, prev;

    top = cap_get_top(v->cap);

    /* Figure out where the next edge is.  We have to translate our current
     * midpoint to the location in the parent.
     */
    mid = cap_get_nsamples(v->cap) / 2;
    prev = cap_prev_edge(top, v->target);

    /* If the previous edge is near the boundary of the capture, snap-scroll
     * to the previous window.
     */
    if (prev <= cap_get_offset(v->cap)) {
        cap_t *old = v->cap;
        v->cap = cap_create_subcap(top, prev - mid, prev + mid);
        if (old != top) {
            cap_dropref(old);
        }
    }

    /* Don't lock the reticle onto the first sample as an edge. */
    if (prev > 0) {
        v->target = prev;
    }

    v->flags |= VIEW_PLOT_DIRTY;
}

// FIXME - Still some edge conditions where zooming and panning don't play nice.
void views_pan_right(struct view *v)
{
    cap_t *top;
    int64_t mid, next;

    top = cap_get_top(v->cap);

    /* Figure out where the next edge is.  We have to translate our current
     * midpoint to the location in the parent.
     */
    mid = cap_get_nsamples(v->cap) / 2;
    next = cap_next_edge(top, v->target);

    /* If the next edge is near the boundary of the capture, snap-scroll
     * to the next window.
     */
//    printf("next: %'ld offset: %'ld nsamples %'ld => total %'ld'\n", next,
//        cap_get_offset(v->cap), cap_get_nsamples(v->cap), cap_get_offset(v->cap) + cap_get_nsamples(v->cap));
    if ((next > cap_get_offset(v->cap) + cap_get_nsamples(v->cap)) &&
        (next < (cap_get_nsamples(top) - 1))) {
        cap_t *old = v->cap;
        v->cap = cap_create_subcap(top, next - mid, next + mid);
        v->flags |= VIEW_PLOT_DIRTY;
        if (old != top)
            cap_dropref(old);
    }

    /* Don't lock the reticle onto the last sample as an edge. */
    if (next < cap_get_nsamples(top) - 1) {
        v->target = next;
    }

    v->flags |= VIEW_PLOT_DIRTY;
}
