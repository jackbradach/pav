#ifndef _GUI_PLOT_H_
#define _GUI_PLOT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "cap.h"

void gui_add_ch_view(cap_t *c);
void gui_refresh_views(void);

void gui_plot_zoom_in(gui_t *g);
void gui_plot_zoom_out(gui_t *g);
void gui_plot_pan_left(gui_t *g);
void gui_plot_pan_right(gui_t *g);

#ifdef __cplusplus
}
#endif

#endif
