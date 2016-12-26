#include "cap.h"
#include "gui.h"
#include "views.h"

void views_add_ch(struct ch_view_list *v, cap_t *c);

void views_create_list(struct ch_view_list **l)
{
    struct ch_view_list *views;

    views = calloc(1, sizeof(struct ch_view_list));
    TAILQ_INIT(views);
    *l = views;
}

void views_populate_from_bundle(cap_bundle_t *b, struct ch_view_list **v)
{
    struct gui *g = gui_get_instance();
    struct ch_view_list *views;
    cap_t *c;

    views_create_list(&views);

    c = cap_bundle_first(g->bundle);
    while (NULL != c) {
        views_add_ch(views, c);
        c = cap_next(c);
    }

    *v = views;
}

void views_add_ch(struct ch_view_list *vl, cap_t *c)
{
    struct gui *g = gui_get_instance();
    struct ch_view *ch;

    ch = calloc(1, sizeof(struct ch_view));
    ch->cap = cap_addref(c);
    ch->sample_selected = cap_get_nsamples(c) / 2;
    plot_from_cap(c, ch->sample_selected, &ch->pl);
    ch->flags &= ~VIEW_PLOT_DIRTY;

    // TODO - this should probably take a size hint
    ch->txt = SDL_CreateTexture(g->renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STREAMING,
                GUI_WIDTH, GUI_HEIGHT);
    plot_to_texture(ch->pl, ch->txt);
    ch->flags &= ~VIEW_TEXTURE_DIRTY;

    TAILQ_INSERT_TAIL(vl, ch, entry);
}

void views_destroy(struct ch_view_list *v)
{
    struct ch_view *cur, *tmp;

    TAILQ_FOREACH_SAFE(cur, v, entry, tmp) {
        TAILQ_REMOVE(v, cur, entry);
        cap_dropref(cur->cap);
        plot_dropref(cur->pl);
        free(cur);
    }
    free(v);
}

void views_refresh(struct ch_view *v)
{
    /* Refresh the plot if the cap changed (eg, subcap view) */
    if (v->flags & VIEW_PLOT_DIRTY) {
        plot_dropref(v->pl);
        plot_from_cap(v->cap, v->sample_selected, &v->pl);
        v->flags &= ~VIEW_PLOT_DIRTY;
        v->flags |= VIEW_TEXTURE_DIRTY;
    }

    /* Refesh the texture if dirty (eg, the size changed) */
    if (v->flags & VIEW_TEXTURE_DIRTY) {
        plot_to_texture(v->pl, v->txt);
        v->flags &= ~VIEW_TEXTURE_DIRTY;
    }
}

void views_zoom_in(struct ch_view *v)
{
    cap_t *old, *top;
    uint64_t z0, z1;

    top = cap_subcap_get_top(v->cap);

    z0 = (cap_get_nsamples(v->cap) / 4);
    z1 = (cap_get_nsamples(v->cap) - z0);
    z0 += cap_get_offset(v->cap);
    z1 += cap_get_offset(v->cap);

    // TODO - critical section
    old = v->cap;
    v->cap = cap_create_subcap(top, z0, z1);
    if (old != top)
        cap_dropref(old);
    v->flags |= VIEW_PLOT_DIRTY;
}

void views_zoom_out(struct ch_view *v)
{
    cap_t *top;
    int64_t z0, z1, zoom_gran;

    top = cap_subcap_get_top(v->cap);

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

void views_pan_left(struct ch_view *v)
{
    cap_t *top;
    int64_t mid, prev;

    top = cap_subcap_get_top(v->cap);

    /* Figure out where the next edge is.  We have to translate our current
     * midpoint to the location in the parent.
     */
    mid = cap_get_nsamples(v->cap) / 2;
    prev = cap_find_prev_edge(top, v->sample_selected);

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
        v->sample_selected = prev;
    }

    v->flags |= VIEW_PLOT_DIRTY;
}

// FIXME - Still some edge conditions where zooming and panning don't play nice.
void views_pan_right(struct ch_view *v)
{
    cap_t *top;
    int64_t mid, next;

    top = cap_subcap_get_top(v->cap);

    /* Figure out where the next edge is.  We have to translate our current
     * midpoint to the location in the parent.
     */
    mid = cap_get_nsamples(v->cap) / 2;
    next = cap_find_next_edge(top, v->sample_selected);

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
        v->sample_selected = next;
    }

    v->flags |= VIEW_PLOT_DIRTY;
}
