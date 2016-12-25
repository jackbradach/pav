#ifndef _GUI_EVENT_H_
#define _GUI_EVENT_H_

#include <stdint.h>

#include "gui.h"

#ifdef __cplusplus
extern "C" {
#endif

enum gui_events {
    GUI_EVENT_DISPLAY,
    __GUI_EVENT_LAST
};
#define GUI_EVENT_MAX (__GUI_EVENT_LAST - 1)

enum display_events {
    DISPLAY_EVENT_REDRAW_ALL
};

/* Non-blocking until program exit! */
void gui_event_loop(void);


#ifdef __cplusplus
}
#endif

#endif
