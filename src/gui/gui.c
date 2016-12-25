/* File: gui.c
 *
 * Protocol Analyzer Viewer - GUI.
 *
 * Author: Jack Bradach <jack@bradach.net>
 *
 * Copyright (C) 2016 Jack Bradach <jack@bradach.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "cairo/cairo.h"

#include "cap.h"
#include "pav.h"
#include "plot.h"
#include "saleae.h"

#include "gui.h"
#include "gui_event.h"
#include "gui_plot.h"

const unsigned GUI_WIDTH = 1024;
const unsigned GUI_HEIGHT = 768;

static void gui_import_file(FILE *fp);
static int init_sdl(void);

/* The GUI structure is a singleton; doesn't make much sense to
 * have a second instance.
 */
struct gui gui_instance = { 0 };
gui_t *gui_get_instance(void)
{
    return &gui_instance;
}

/* Sets the 'quit' flag, which will cause the GUI to clean itself up. */
void gui_quit(void)
{
    gui_instance.quit = true;
}

/* State of GUI */
bool gui_active(void)
{
    return !gui_instance.quit;
}

/* Entry point for the GUI; initializes SDL, sets up ANY
 * other structures, and then enters the event loop until quit.
 */
void gui_start(struct pav_opts *opts)
{
    struct gui *gui = gui_get_instance();
    int rc;

    gui->opts = opts;

    rc = init_sdl();
    if (rc < 0) {
        fprintf(stderr, "Unable to initialze SDL: %s\n", SDL_GetError());
        return;
    }

    gui_import_file(opts->fin);
    gui_plot_all(gui);

    gui->cap = cap_addref(cap_bundle_first(gui->bundle));
    gui->quit = false;

    //display_init();

    /* Blocking */
    gui_event_loop();
}

static void gui_import_file(FILE *fp)
{
    struct gui *g = gui_get_instance();
    if (g->bundle) {
        cap_bundle_dropref(g->bundle);
    }

    saleae_import_analog(fp, &g->bundle);
}

static int init_sdl(void)
{
    struct gui *gui = gui_get_instance();
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int rc;

    rc = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    if (rc < 0) {
        return rc;
    }
    atexit(SDL_Quit);

    rc = SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    if (SDL_FALSE == rc) {
        printf("Couldn't set up SDL hinting: %s\n", SDL_GetError());
        return rc;
    }

    window = SDL_CreateWindow("pav",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              GUI_WIDTH, GUI_HEIGHT,
                              SDL_WINDOW_OPENGL);
    if (NULL == window) {
        return -1;
    }
    gui->window = window;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (NULL == renderer) {
        return -1;
    }
    gui->renderer = renderer;

    texture = SDL_CreateTexture(renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_STREAMING,
                GUI_WIDTH, GUI_HEIGHT);
    if (NULL == texture) {
        SDL_Log("SDL_CreateTexture() failed: %s", SDL_GetError());
        return -1;
    }

    gui->texture = texture;

    return 0;
}
