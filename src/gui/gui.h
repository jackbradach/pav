/* File: gui.h
 *
 * Protocol Analyzer Viewer - GUI (headers)
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
#ifndef _GUI_H_
#define _GUI_H_

#include <SDL2/SDL.h>

#include "cap.h"
#include "pav.h"
#include "plot.h"
#include "views.h"

extern unsigned GUI_WIDTH;
extern unsigned GUI_HEIGHT;



struct gui {
    SDL_Window *window;
	SDL_Renderer *renderer;
    // TODO - drop texture and have a 'render list' instead
    SDL_Texture *texture;
    bool quit;
    struct pav_opts *opts;

    struct ch_view_list *views;
    struct ch_view *view_active;

    cap_bundle_t *bundle;

};
typedef struct gui gui_t;

void gui_start(struct pav_opts *opts);
gui_t *gui_get_instance(void);
void gui_quit(void);
bool gui_active(void);
void gui_draw(void);
#endif
