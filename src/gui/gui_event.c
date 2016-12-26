#include <SDL2/SDL.h>

#include "display.h"
#include "gui.h"
#include "gui_plot.h"

#include "gui_event.h"

#define EVENT_TIMEOUT_MS 500

static void process_event(SDL_Event *e);
static void process_keyboard_input(SDL_KeyboardEvent *key);
static void process_user_event(SDL_Event *e);
static int register_user_events(void);

static uint32_t event_type_map[GUI_EVENT_MAX + 1] = { 0 };

typedef void (*event_cb)(SDL_UserEvent *e);


/* Registers custom event types, sets up gui refresh */
void gui_event_loop_init(struct gui *gui)
{
    if (0 != register_user_events()) {
        fprintf(stderr, "Unable to register event handlers: %s\n", SDL_GetError());
        gui->quit = true;
        return;
    }
}

/* Registers custom user event types with SDL */
static int register_user_events(void)
{
    uint32_t etype_start = SDL_RegisterEvents(GUI_EVENT_MAX + 1);
    if (etype_start == ((uint32_t) - 1)) {
        return -1;
    }

    for (int i = 0; i < GUI_EVENT_MAX + 1; i++) {
        event_type_map[i] = etype_start + i;
    }

    return 0;
}

void gui_event_loop(void)
{
    while (gui_active()) {
        SDL_Event e;
        int rc;
        // TODO - use waiteventtimeout after refreshes are timer based.
        //    rc = SDL_WaitEventTimeout(&e, EVENT_TIMEOUT_MS);
        rc = SDL_WaitEvent(&e);
        if (!rc) {
            fprintf(stderr, "Event loop error: %s\n", SDL_GetError());
            gui_quit();
            continue;
        }

        process_event(&e);
        display_refresh(0,0);
    }

}

static void process_event(SDL_Event *e)
{
    switch (e->type) {

    case SDL_KEYDOWN:
    case SDL_KEYUP:
        process_keyboard_input(&e->key);
        break;
    case SDL_QUIT:
        gui_quit();
        break;
    default:
        /* Try to handle as a user event */
        process_user_event(e);
        break;
    }

    /* Refresh! */
    // TODO - move to event callback
}

static void process_user_event(SDL_Event *e)
{


}

static void process_keyboard_input(SDL_KeyboardEvent *key)
{
    struct gui *gui = gui_get_instance();
    switch (key->type) {
    case SDL_KEYDOWN:
        switch(key->keysym.sym) {

        /* Quit! */
        case SDLK_q:
            gui->quit = true;
            break;
        /* Enbiggen! */
        case SDLK_z:
            views_zoom_in(gui->view_active);
            break;
        /* Re-smallify */
        case SDLK_x:
            views_zoom_out(gui->view_active);
            break;
        case SDLK_LEFT:
            views_pan_left(gui->view_active);
            break;
        case SDLK_RIGHT:
            views_pan_right(gui->view_active);
            break;
        case SDLK_UP: {
            if (gui->view_active == TAILQ_FIRST(gui->views)) {
                gui->view_active = TAILQ_LAST(gui->views, ch_view_list);
            } else {
                gui->view_active = TAILQ_PREV(gui->view_active, ch_view_list, entry);
            }
            break;
        }
        case SDLK_DOWN:
            if (gui->view_active == TAILQ_LAST(gui->views, ch_view_list)) {
                gui->view_active = TAILQ_FIRST(gui->views);
            } else {
                gui->view_active = TAILQ_NEXT(gui->view_active, entry);
            }
            break;
        }
        break;
    case SDL_KEYUP:
        break;
    default:
        break;
    }
}
