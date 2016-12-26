#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <SDL2/SDL.h>

void display_init(void);
uint32_t display_refresh(uint32_t interval, void *param);


#endif
