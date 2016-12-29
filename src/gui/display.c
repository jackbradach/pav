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
    // should possibly drop.
}

void display_refresh(void)
{
    struct gui *g = gui_get_instance();
    view_t *v;
    int idx = 0;

    SDL_RenderClear(g->renderer);

    v = views_first(g->views);
    while (NULL != v) {
        SDL_Rect dstrect;
        SDL_Texture *txt;
        int w, h;

        views_refresh(v);

        if (v == g->active_view) {
            glUseProgram(g->shader);
        } else {
            glUseProgram(0);
        }

        txt = views_get_texture(v);
        SDL_QueryTexture(txt, NULL, NULL, &w, &h);
        dstrect.x = 0;
        dstrect.y = idx * h;
        dstrect.w = w;
        dstrect.h = h;
        SDL_RenderCopy(g->renderer, txt, NULL, &dstrect);
        idx++;
        v = views_next(v);
    }

    SDL_RenderPresent(g->renderer);
}
