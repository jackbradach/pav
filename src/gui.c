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

    gui->cap = cap_bundle_first(gui->bundle);

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
    cap_t *c, *sc;
    plot_t *pl;
    uint64_t z0, z1;

    c = g->cap;

    z0 = (cap_get_nsamples(c) / 4);
    z1 = (cap_get_nsamples(c) - z0);

    printf("Zoom in: %'lu samples centered @ %'lu\n", z1 - z0, (z1 - z0) / 2);
    sc = cap_create_subcap(c, z0, z1);
    plot_from_cap(sc, &pl);
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    //cap_dropref(g->cap);
    g->cap = sc;
}

void gui_zoom_out(struct gui *g)
{
    cap_t *c, *sc;
    plot_t *pl;
    int64_t z0, z1;

    c = cap_get_parent(g->cap);

    /* If there's no parent, this is the top-level zoom! */
    if (NULL == c) {
        printf("Unable to zoom out further (Top)!\n");
        return;
    }
#if 0
    printf("offset: %lu\n", cap_get_offset(sc));
    z0 = (cap_get_nsamples(sc) / 2);
    z1 = z0 + (cap_get_nsamples(sc));

    printf("z0: %ld z1: %ld\n", z0, z1);
    z0 -= cap_get_offset(sc);
    z1 += cap_get_offset(sc);

    printf("-samples: %lu tgt: %ld off: %lu z0: %ld z1: %ld\n",
        cap_get_nsamples(sc), (z1 - z0), cap_get_offset(sc), z0, z1);
#endif
    z0 = cap_get_offset(c);
    z1 = cap_get_offset(c) + cap_get_nsamples(c);
    printf("Zoom out: %'lu samples centered @ %'lu\n", z1 - z0, (z1 - z0) / 2);
    plot_from_cap(c, &pl);
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    cap_dropref(g->cap);
    g->cap = c;
}

void gui_pan_left(struct gui *g)
{
    cap_t *old, *top, *sc;
    plot_t *pl;
    uint64_t mid, from, prev;

    old = g->cap;
    top = cap_subcap_get_top(old);

    /* Can't pan at the highest zoom. */
    if (top == old)
        return;

    /* Figure out where the next edge is.  We have to translate our current
     * midpoint to the location in the parent.
     */
    mid = cap_get_nsamples(old) / 2;
    from = mid + cap_get_offset(old);
    prev = cap_find_prev_edge(top, from);

    /* If the next edge is near the boundary of the capture, don't shift. */
    if ((prev - mid) < 0) {
        return;
    }

    sc = cap_create_subcap(top, prev - mid, prev + mid);
    plot_from_cap(sc, &pl);
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    g->cap = sc;
    cap_dropref(old);
}

void gui_pan_right(struct gui *g)
{
    cap_t *old, *top, *sc;
    plot_t *pl;
    uint64_t mid, next, from;

    old = g->cap;
    top = cap_subcap_get_top(old);

    /* Can't pan at the highest zoom. */
    if (top == old)
        return;

    /* Figure out where the next edge is.  We have to translate our current
     * midpoint to the location in the parent.
     */
    mid = cap_get_nsamples(old) / 2 + cap_get_offset(old);
    next = cap_find_next_edge(top, mid);

    /* If the next edge is near the boundary of the capture, don't shift. */
    if ((next + mid) > (cap_get_nsamples(top))) {
        printf("*BUMP*\n");
        return;
    }

    sc = cap_create_subcap(top, next - mid, next + mid);
    plot_from_cap(sc, &pl);
    plot_to_texture(pl, g->texture);
    plot_dropref(pl);

    /* Replace active capture */
    g->cap = sc;
    cap_dropref(old);

    printf("Right!\n");
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
