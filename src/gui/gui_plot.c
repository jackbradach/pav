#include <stdint.h>
#include "cap.h"
#include "gui.h"
#include "plot.h"


void gui_plot_all(struct gui *g)
{
    cap_t *cap;
    plot_t *pl;

    cap = cap_bundle_first(g->bundle);
    plot_from_cap(cap, &pl);
    plot_set_reticle(pl, cap_get_nsamples(cap) / 2);
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);
}

void gui_plot_zoom_in(struct gui *g)
{
    cap_t *sc, *top;
    plot_t *pl;
    uint64_t z0, z1;

    top = cap_subcap_get_top(g->cap);

    z0 = (cap_get_nsamples(g->cap) / 4);
    z1 = (cap_get_nsamples(g->cap) - z0);
    z0 += cap_get_offset(g->cap);
    z1 += cap_get_offset(g->cap);


    sc = cap_create_subcap(top, z0, z1);
    plot_from_cap(sc, &pl);
    plot_set_reticle(pl, cap_get_nsamples(sc) / 2);
//    printf("Zoom out: %'lu samples, Reticle @ %'lu\n", cap_get_nsamples(sc), plot_get_reticle(pl));
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    cap_dropref(g->cap);
    g->cap = sc;
}

void gui_plot_zoom_out(struct gui *g)
{
    cap_t *parent, *top, *cap;
    plot_t *pl;
    int64_t z0, z1, zoom_gran;

    parent = cap_get_parent(g->cap);
    top = cap_subcap_get_top(g->cap);

    /* If there's no parent, this is the top-level zoom! */
    if (!parent) {
        printf("Unable to zoom out further (Top)!\n");
        return;
    }

    zoom_gran = cap_get_nsamples(g->cap) / 2;
    z0 = cap_get_offset(g->cap) - zoom_gran;
    z1 = cap_get_offset(g->cap) + cap_get_nsamples(g->cap) + zoom_gran;

    if (z0 < 0) {
        z0 = 0;
    }

    if (z1 > (cap_get_nsamples(top) - 1)) {
        z1 = cap_get_nsamples(top) - 1;
    }

    if ((z0 == 0) && (z1 == (cap_get_nsamples(top) - 1))) {
        cap = top;
    } else {
        cap = cap_create_subcap(top, z0, z1);
    }

    plot_from_cap(cap, &pl);
    plot_set_reticle(pl, cap_get_nsamples(cap) / 2);
//    printf("Zoom out: %'lu samples, Reticle @ %'lu\n", cap_get_nsamples(cap), plot_get_reticle(pl));
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    // FIXME - jbradach - 2016/12/24 - this is leaking memory, need to
    // have it *not* drop all references to the top!
    // cap_dropref(g->cap);
    g->cap = cap;
}

void gui_plot_pan_left(struct gui *g)
{
    cap_t *top, *sc, *parent;
    plot_t *pl;
    int64_t mid, from, prev;

    parent = cap_get_parent(g->cap);
    top = cap_subcap_get_top(g->cap);

    /* Can't pan at the highest zoom. */
    if (!parent) {
        printf("Can't pan at top-level (try zooming in!)\n");
        return;
    }

    /* Figure out where the next edge is.  We have to translate our current
     * midpoint to the location in the parent.
     */
    mid = cap_get_nsamples(g->cap) / 2;
    from = mid + cap_get_offset(g->cap);
    prev = cap_find_prev_edge(top, from);

    /* If the next edge is near the boundary of the capture, don't shift. */
    // FIXME - 2016/12/24 - jbradach - the reticle can still shift, need to
    // FIXME - track in the capture.
    if ((prev - mid) < 0) {
        printf("*BUMP* (No more left to pan!)\n");
        return;
    }

    sc = cap_create_subcap(parent, prev - mid, prev + mid);
    plot_from_cap(sc, &pl);
    plot_set_reticle(pl, prev - cap_get_offset(sc));
//    printf("Prev edge @ %'lu, Reticle @ %'lu\n", prev, plot_get_reticle(pl));
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    cap_dropref(g->cap);
    g->cap = sc;
}

void gui_plot_pan_right(struct gui *g)
{
    cap_t *top, *sc, *parent;
    plot_t *pl;
    int64_t mid, next, from;

    parent = cap_get_parent(g->cap);
    top = cap_subcap_get_top(g->cap);

    /* Can't pan at the highest zoom. */
    if (!parent) {
        printf("Can't pan at top-level (try zooming in!)\n");
        return;
    }

    /* Figure out where the next edge is.  We have to translate our current
     * midpoint to the location in the parent.
     */
    mid = cap_get_nsamples(g->cap) / 2;
    from = mid + cap_get_offset(g->cap);
    next = cap_find_next_edge(top, from);

    /* If the next edge is near the boundary of the capture, don't shift. */
    // FIXME - 2016/12/24 - jbradach - the reticle can still shift, need to
    // FIXME - track in the capture.
    if ((next + mid) >= (cap_get_nsamples(top) - 1)) {
        printf("*BUMP* (No more right to pan!)\n");
        return;
    }

    sc = cap_create_subcap(top, next - mid, next + mid);
    plot_from_cap(sc, &pl);
    plot_set_reticle(pl, next - cap_get_offset(sc));
//    printf("Next edge @ %'lu, Reticle @ %'lu\n", next, plot_get_reticle(pl));
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    cap_dropref(g->cap);
    g->cap = sc;
}
