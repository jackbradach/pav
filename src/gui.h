#ifndef _GUI_H_
#define _GUI_H_

#include <SDL2/SDL.h>

typedef struct gui_ctx gui_ctx_t;

void gui_init(gui_ctx_t **uctx);
void gui_draw(gui_ctx_t *gui);
SDL_Texture *gui_get_texture(struct gui_ctx *gui);

#endif
