#ifndef _PLOT_H_
#define _PLOT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cap.h"
#include "gui.h"
#include "cairo/cairo.h"

typedef struct plot plot_t;
void plot_from_cap(cap_t *cap, plot_t **plot);
void plot_analog_cap_gui(struct gui_ctx *gui, struct cap_analog *acap, unsigned idx_start, unsigned idx_end);

plot_t *plot_create(void);
plot_t *plot_addref(plot_t *p);
unsigned plot_getref(plot_t *p);
void plot_dropref(plot_t *p);

void plot_set_xlabel(plot_t *p, const char *xlabel);
void plot_set_ylabel(plot_t *p, const char *ylabel);
void plot_set_title(plot_t *p, const char *title);

const char *plot_get_xlabel(plot_t *p);
const char *plot_get_ylabel(plot_t *p);
const char *plot_get_title(plot_t *p);


void plot_to_wxwidgets(plot_t *p);
void plot_to_cairo_surface(plot_t *p, cairo_surface_t *cs);

#ifdef __cplusplus
}
#endif

#endif
