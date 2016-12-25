#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "gui.h"

#include "display.h"

const uint32_t refresh_delay_ms = 10;
uint32_t display_refresh(uint32_t interval, void *param);

void display_init(void)
{
    //SDL_AddTimer(refresh_delay_ms, display_refresh, NULL);

}

uint32_t display_refresh(uint32_t interval, void *param)
{
    struct gui *gui = gui_get_instance();
    SDL_RenderClear(gui->renderer);
    // XXX - this can eventually walk a list to assemble a 'scene'
    SDL_RenderCopy(gui->renderer, gui->texture, NULL, NULL);
    SDL_RenderPresent(gui->renderer);
    fflush(stdout);
    return (interval);
}
