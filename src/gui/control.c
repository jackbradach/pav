/* Control panel routines */
#include <math.h>
#include <SDL2/SDL.h>

// Control panel drawn on texture on right
// Use SDL drawing routines?
#include "gui.h"

#define COLOR_BLACK 0x0
#define COLOR_WHITE 0xFFFFFF
#define COLOR_RED 0xAB3126
#define COLOR_CYAN 0x66DAFF
#define COLOR_PURPLE 0xBB3FB8
#define COLOR_GREEN 0x55CE58
#define COLOR_BLUE 0x1D0E97
#define COLOR_YELLOW 0xEAF57C
#define COLOR_ORANGE 0xB97418
#define COLOR_BROWN 0x785300
#define COLOR_LIGHT_RED 0xDD9387
#define COLOR_LIGHT_GREEN 0xB0F4AC
#define COLOR_LIGHT_BLUE 0xAA9DEF
#define COLOR_DARK_GRAY 0x5B5B5B
#define COLOR_MEDIUM_GRAY 0x8B8B8B
#define COLOR_LIGHT_GRAY 0xB8B8B8

#define ALPHA_OPAQUE 0xFF000000

void control_create(void)
{
    uint8_t r, g, b, a;
    SDL_Renderer *rnd = gui_get_renderer();
    SDL_Texture *txt = gui_get_texture();
    uint32_t fmt;
    SDL_PixelFormat *pixfmt;
    int w, h, access;

    SDL_SetRenderTarget(rnd, txt);
    SDL_QueryTexture(txt, &fmt, &access, &w, &h);

    pixfmt = SDL_AllocFormat(fmt);

    SDL_GetRGBA(COLOR_GREEN | ALPHA_OPAQUE, pixfmt, &r, &g, &b, &a);
    SDL_SetRenderDrawColor(rnd, r, g, b, a);
    //SDL_RenderFillRect(rnd, NULL);

    printf("WxH: %dx%d\n", w, h);
    SDL_Rect outline = {0, 0, w, h};
#if 0
    SDL_RenderDrawLine(rnd, 0, 0, w, 0);
    SDL_RenderDrawLine(rnd, w, 0, w, h);
    SDL_RenderDrawLine(rnd, 0, 0, 0, h);
    SDL_RenderDrawLine(rnd, 0, h, w, h);
#endif
    SDL_RenderDrawRect(rnd, &outline);
    SDL_Rect dstrect = {1024 - w, 0, w, h};
    // load font
    // title across top
    SDL_SetRenderTarget(rnd, NULL);
    SDL_RenderCopy(rnd, txt, NULL, &dstrect);

    SDL_RenderPresent(rnd);
}
