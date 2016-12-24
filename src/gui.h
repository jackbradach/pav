#ifndef _GUI_H_
#define _GUI_H_

#include <SDL2/SDL.h>

typedef struct gui gui_t;

void gui_init(gui_t **ugui);
void gui_draw(gui_t *gui);
SDL_Texture *gui_get_texture(struct gui *gui);

#endif
