#ifndef _GUI_PLOT_H_
#define _GUI_PLOT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void gui_plot_all(gui_t *g);
void gui_plot_zoom_in(gui_t *g);
void gui_plot_zoom_out(gui_t *g);
void gui_plot_pan_left(gui_t *g);
void gui_plot_pan_right(gui_t *g);

#ifdef __cplusplus
}
#endif

#endif
