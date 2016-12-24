#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "cairo/cairo.h"

#include "cap.h"
#include "gui.h"
#include "pav.h"
#include "plot.h"
#include "saleae.h"

const unsigned GUI_WIDTH = 1024;
const unsigned GUI_HEIGHT = 768;

struct gui {
    SDL_Window *window;
	SDL_Renderer *renderer;
    SDL_Texture *texture;
    bool quit;
    struct pav_opts *opts;
    cap_bundle_t *bundle;

    /* Visable capture */
    cap_t *cap;

    // event callback?
};

static void gui_event_loop(struct gui *gui);
static void gui_import_file(struct gui *g, FILE *fp);
static void gui_plot_all(struct gui *g);

static void keyboard_input(struct gui *gui, SDL_KeyboardEvent *key);
static void init_sdl(struct gui *gui);

void gui_start(struct pav_opts *opts)
{
    struct gui *gui;

    gui = calloc(1, sizeof(struct gui));
    init_sdl(gui);

    gui->opts = opts;

    gui_import_file(gui, opts->fin);
    gui_plot_all(gui);

    gui->cap = cap_addref(cap_bundle_first(gui->bundle));

    /* Blocking */
    while (!gui->quit) {
        gui_event_loop(gui);
    }
}

static void gui_import_file(struct gui *g, FILE *fp)
{
    if (g->bundle) {
        cap_bundle_dropref(g->bundle);
    }

    saleae_import_analog(fp, &g->bundle);
}

void gui_plot_all(struct gui *g)
{
    cap_t *cap;
    plot_t *pl;

    cap = cap_bundle_first(g->bundle);
    plot_from_cap(cap, &pl);
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);
}

void gui_zoom_in(struct gui *g)
{
    cap_t *sc, *top;
    plot_t *pl;
    uint64_t z0, z1;

    top = cap_subcap_get_top(g->cap);

    z0 = (cap_get_nsamples(g->cap) / 4);
    z1 = (cap_get_nsamples(g->cap) - z0);
    z0 += cap_get_offset(g->cap);
    z1 += cap_get_offset(g->cap);

    printf("Zoom in: %'lu samples centered @ %'lu\n", z1 - z0, (z1 - z0) / 2);

    sc = cap_create_subcap(top, z0, z1);
    plot_from_cap(sc, &pl);
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    cap_dropref(g->cap);
    g->cap = sc;
}

void gui_zoom_out(struct gui *g)
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

    printf("Zoom out: %'lu samples centered @ %'lu\n", z1 - z0, (z1 - z0) / 2);
    if ((z0 == 0) && (z1 == (cap_get_nsamples(top) - 1))) {
        cap = top;
    } else {
        printf("Using subcap!\n");
        cap = cap_create_subcap(top, z0, z1);
    }

    plot_from_cap(cap, &pl);
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    // FIXME - jbradach - 2016/12/24 - this is leaking memory, need to
    // have it *not* drop all references to the top!
    // cap_dropref(g->cap);
    g->cap = cap;
}

void gui_pan_left(struct gui *g)
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
    if ((prev - mid) < 0) {
        printf("*BUMP* (No more left to pan!)\n");
        return;
    }

    printf("Prev edge @ %'lu\n", prev);

    sc = cap_create_subcap(parent, prev - mid, prev + mid);
    plot_from_cap(sc, &pl);
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    cap_dropref(g->cap);
    g->cap = sc;
}

void gui_pan_right(struct gui *g)
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

    printf("Next edge @ %'lu\n", next);

    /* If the next edge is near the boundary of the capture, don't shift. */
    if ((next + mid) >= (cap_get_nsamples(top) - 1)) {
        printf("*BUMP* (No more right to pan!)\n");
        return;
    }

    sc = cap_create_subcap(top, next - mid, next + mid);
    plot_from_cap(sc, &pl);
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    cap_dropref(g->cap);
    g->cap = sc;
}

static void init_sdl(struct gui *gui)
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int rc;

    rc = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    assert(rc >= 0);
    atexit(SDL_Quit);

    rc = SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    if (0 == rc) {
        printf("Couldn't set up SDL hinting: %s\n", SDL_GetError());
    }

    window = SDL_CreateWindow("pav",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              GUI_WIDTH, GUI_HEIGHT,
                              SDL_WINDOW_OPENGL);
    assert(NULL != window);
    gui->window = window;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    assert(NULL != renderer);
    gui->renderer = renderer;

    texture = SDL_CreateTexture(renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STREAMING,
                GUI_WIDTH, GUI_HEIGHT);
    if (NULL == texture) {
        SDL_Log("SDL_CreateTexture() failed: %s", SDL_GetError());
        exit(1);
    }

    gui->texture = texture;
}

// XXX - this can eventually walk a list to assemble a 'scene'
void gui_draw(struct gui *gui)
{
    SDL_RenderClear(gui->renderer);
    SDL_RenderCopy(gui->renderer, gui->texture, NULL, NULL);
    SDL_RenderPresent(gui->renderer);
}


void gui_event_loop(struct gui *gui)
{
    SDL_Event e;

    while (SDL_PollEvent((&e))) {
        switch (e.type) {

        case SDL_QUIT:
            gui->quit = true;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            keyboard_input(gui, &e.key);
            break;

        }
    }

    /* Refresh! */
    gui_draw(gui);
}

static void keyboard_input(struct gui *gui, SDL_KeyboardEvent *key)
{
    switch (key->type) {
    case SDL_KEYDOWN:
        switch(key->keysym.sym) {

        /* Quit! */
        case SDLK_q:
            gui->quit = true;
            break;
        /* Enbiggen! */
        case SDLK_z:
            gui_zoom_in(gui);
            break;
        /* Re-smallify */
        case SDLK_x:
            gui_zoom_out(gui);
            break;
        case SDLK_LEFT:
            gui_pan_left(gui);
            break;
        case SDLK_RIGHT:
            gui_pan_right(gui);
            break;
        }
        break;
    case SDL_KEYUP:
        break;
    default:
        break;
    }
}
