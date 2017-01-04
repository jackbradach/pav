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

#include "display.h"
#include "gui.h"
#include "gui_event.h"
#include "gui_plot.h"
#include "views.h"
#include "shaders.h"


unsigned GUI_WIDTH = 1024;
unsigned GUI_HEIGHT = 768;

static int init_sdl_window(SDL_Window **window);
static void gui_views_from_fp(FILE *fp);
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

    gui_views_from_fp(opts->fin);

    gui->active_view = views_first(gui->views);

    gui->quit = false;

    display_init();

    {
        GLint p;
        glewInit();
        p = shader_compile_program("test.vert", "test.frag");
        printf("p = %d\n", p);
        gui->shader = p;
    }

    control_init();
    control_create();

    /* Blocking */
    gui_event_loop();
}

/* Create a capture bundle from a file.  If bun points to
 * an existing bundle, it'll be swapped out and dropreffed.
 */
static void gui_views_from_fp(FILE *fp)
{
    struct gui *g = gui_get_instance();
    cap_bundle_t *b1, *b2, *old;
    struct pav_opts *opts = g->opts;

    saleae_import_analog(fp, &b1);

    b2 = cap_bundle_create();
    for (int i = 0; i < opts->duplicate + 1; i++) {
        cap_t *c = cap_bundle_first(b1);
        cap_clone_to_bundle(b2, c, opts->nloops, i * opts->skew_us);
    }
    cap_bundle_dropref(b1);

    old = g->bundle;
    g->bundle = b2;
    cap_bundle_dropref(old);

    views_populate_from_bundle(g->bundle, &g->views);
}

static int init_sdl(void)
{
    struct gui *gui = gui_get_instance();
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

    rc = init_sdl_window(&gui->window);
    if (rc < 0) {
        fprintf(stderr, "Create window: %s\n", SDL_GetError());
        return rc;
    }

    renderer = SDL_CreateRenderer(gui->window, -1, SDL_RENDERER_ACCELERATED);
    if (NULL == renderer) {
        return -1;
    }
    gui->renderer = renderer;

    texture = SDL_CreateTexture(renderer,
                SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_TARGET,
                GUI_WIDTH / 8, GUI_HEIGHT);
    if (NULL == texture) {
        SDL_Log("SDL_CreateTexture() failed: %s", SDL_GetError());
        return -1;
    }
    gui->texture = texture;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
    SDL_GL_SetSwapInterval(1);
    gui->glctx = SDL_GL_CreateContext(gui->window);

    /* Init GLEW */
    glewExperimental = GL_TRUE;
    rc = glewInit();
    if (GLEW_OK != rc) {
        SDL_Log("Couldn't initialize GLEW!\n");
        exit(EXIT_FAILURE);
    }

    /* Init GLUT */
    // FIXME - this is supposed to parse argc/argp before our own argp;
    // need to change gui_start() to handle command parsing?
    {
        char *bogus_argv[1] = { "bogus" };
        int bogus_argc = 1;
        glutInit(&bogus_argc, bogus_argv);
    }


    return 0;
}

SDL_Window *gui_get_window(void)
{
    struct gui *g = gui_get_instance();
    return g->window;
}

void gui_get_size(int *w, int *h)
{
    struct gui *g = gui_get_instance();
    SDL_GetWindowSize(g->window, w, h);
}


static int init_sdl_window(SDL_Window **window)
{
    uint32_t flags = SDL_WINDOW_OPENGL |
                     SDL_WINDOW_RESIZABLE;
    SDL_Window *w;

    w = SDL_CreateWindow("pav",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              GUI_WIDTH, GUI_HEIGHT,
                              flags);
    if (NULL == w) {
        return -1;
    }
    *window = w;
    return 0;
}

SDL_Texture *gui_get_texture(void)
{
    struct gui *g = gui_get_instance();
    return g->texture;
}

SDL_Renderer *gui_get_renderer(void)
{
    struct gui *g = gui_get_instance();
    return g->renderer;
}

views_t *gui_get_views(void)
{
    struct gui *g = gui_get_instance();
    return g->views;
}

view_t *get_active_view(void)
{
    struct gui *g = gui_get_instance();
    return g->active_view;
}
