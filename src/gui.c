#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <SDL2/SDL.h>

#include "cairo/cairo.h"

#include "gui.h"

const unsigned GUI_WIDTH = 640;
const unsigned GUI_HEIGHT = 480;

struct gui_ctx {
    SDL_Window *window;
	SDL_Renderer *renderer;
    SDL_Texture *texture;
    // event callback?
};


SDL_Texture *gui_get_texture(struct gui_ctx *gui)
{
    return gui->texture;
}

//
static void cairo_draw_texture(SDL_Texture *texture)
{
    cairo_surface_t *cairo_surface;
    cairo_t *cairo_ctx;
    void *pixels;
    int pitch;
    int w, h;

    SDL_QueryTexture(texture, NULL, NULL, &w, &h);

    SDL_LockTexture(texture, NULL, &pixels, &pitch);
    cairo_surface = cairo_image_surface_create_for_data(
                        pixels, CAIRO_FORMAT_ARGB32,
                        w, h, pitch);
    cairo_ctx = cairo_create (cairo_surface);
    cairo_surface_destroy (cairo_surface);
    cairo_scale (cairo_ctx, w, h);
//    draw (cairo_ctx);
    SDL_UnlockTexture(texture);
    cairo_destroy (cairo_ctx);
}

static uint32_t cb_gui_repaint(uint32_t interval, void *param)
{

}


static int init_sdl(struct gui_ctx *gui)
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int rc;

    rc = SDL_Init(SDL_INIT_VIDEO);
    assert(rc >= 0);

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

    return(0);
}

// XXX - this can eventually walk a list to assemble a 'scene'
void gui_draw(struct gui_ctx *gui)
{
    SDL_RenderClear(gui->renderer);
    SDL_RenderCopy(gui->renderer, gui->texture, NULL, NULL);
    SDL_RenderPresent(gui->renderer);
}

void gui_init(struct gui_ctx **ugui)
{
    struct gui_ctx *gui;

    int rc;

    gui = calloc(1, sizeof(struct gui_ctx));
    rc = init_sdl(gui);
    assert (rc >= 0);

    *ugui = gui;
}
