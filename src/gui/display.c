#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "display.h"
#include "gui.h"
#include "views.h"


const uint32_t refresh_delay_ms = 10;


void display_init(void)
{
    //SDL_AddTimer(refresh_delay_ms, display_refresh, NULL);

}

uint32_t display_refresh(uint32_t interval, void *param)
{
    struct gui *g = gui_get_instance();
    struct ch_view *view;

    SDL_RenderClear(g->renderer);

    TAILQ_FOREACH(view, g->views, entry) {
        views_refresh(view);
        SDL_RenderCopy(g->renderer, view->txt, NULL, NULL);
    }

    SDL_RenderPresent(g->renderer);

    return (interval);
}
