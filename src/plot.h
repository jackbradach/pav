#ifndef _PLOT_H_
#define _PLOT_H_

#include "cap.h"
#include "gui.h"

typedef struct plot_data plot_data_t;
void plot_analog_cap(struct cap_analog *acap, unsigned idx_start, unsigned idx_end);
void plot_analog_cap_gui(struct gui_ctx *gui, struct cap_analog *acap, unsigned idx_start, unsigned idx_end);


void plot_data_set_xlabel(struct plot_data *p, const char *xlabel);
void plot_data_set_ylabel(struct plot_data *p, const char *ylabel);
void plot_data_set_title(struct plot_data *p, const char *title);

const char *plot_data_get_xlabel(struct plot_data *p);
const char *plot_data_get_ylabel(struct plot_data *p);
const char *plot_data_get_title(struct plot_data *p);
#endif
