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
    int idx = 0;

    SDL_RenderClear(g->renderer);

    TAILQ_FOREACH(view, g->views, entry) {
        SDL_Rect dstrect;
        int w, h;
        views_refresh(view);
        if (view == g->view_active) {
            glUseProgram(g->shader);
        } else {
            glUseProgram(0);
        }
        SDL_QueryTexture(view->txt, NULL, NULL, &w, &h);
        dstrect.x = 0;
        dstrect.y = idx * h;
        dstrect.w = w;
        dstrect.h = h;
        SDL_RenderCopy(g->renderer, view->txt, NULL, &dstrect);
        idx++;
    }

    SDL_RenderPresent(g->renderer);

    return (interval);
}
