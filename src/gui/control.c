/* Control panel routines */
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL_ttf.h>

// Control panel drawn on texture on right
// Use SDL drawing routines?
#include "gui.h"

#include "res_fonts_C64_Pro_ttf.h"

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

static void load_fonts(void);

TTF_Font *font = NULL;

static inline SDL_Color argb_hex_to_SDL_Color(uint32_t hex)
{
    SDL_Color c = {
        .a = (hex & 0xFF000000) >> 24,
        .r = (hex & 0xFF0000) >> 16,
        .g = (hex & 0xFF00) >> 8,
        .b = (hex & 0xFF)
    };

    return c;
}

void control_init(void)
{
    TTF_Init();
    load_fonts();
}


void load_fonts(void)
{
    SDL_RWops *rwop =
        SDL_RWFromMem(res_fonts_C64_Pro_ttf, res_fonts_C64_Pro_ttf_size);
    font = TTF_OpenFontRW(rwop, 0, 12);

}

void control_create(void)
{
    const double phi = (1.0 + sqrt(5.0)) / 2.0;
    float x0, y0;
    float panel_width, panel_height;
    int win_x, win_y;
    uint8_t r, g, b, a;
    SDL_Renderer *rnd = gui_get_renderer();
    SDL_Texture *txt = gui_get_texture();
    uint32_t fmt;
    SDL_PixelFormat *pixfmt;
    int w, h, access;

    gui_get_size(&win_x, &win_y);

    SDL_SetRenderTarget(rnd, txt);
    SDL_QueryTexture(txt, &fmt, &access, &w, &h);

    pixfmt = SDL_AllocFormat(fmt);

    SDL_GetRGBA(COLOR_BROWN | ALPHA_OPAQUE, pixfmt, &r, &g, &b, &a);
    SDL_SetRenderDrawColor(rnd, r, g, b, a);
    //SDL_RenderFillRect(rnd, NULL);

    panel_width = win_x - (win_x / phi);
    SDL_Rect outline = {0, 0, w, h};
#if 0
    SDL_RenderDrawLine(rnd, 0, 0, w, 0);
    SDL_RenderDrawLine(rnd, w, 0, w, h);
    SDL_RenderDrawLine(rnd, 0, 0, 0, h);
    SDL_RenderDrawLine(rnd, 0, h, w, h);
#endif
    SDL_RenderFillRect(rnd, &outline);
    // load font
    // title across top
    {
        SDL_Surface *ts = TTF_RenderText_Blended(font, "Signal View", argb_hex_to_SDL_Color(COLOR_WHITE));
        SDL_Texture *tt = SDL_CreateTextureFromSurface(rnd, ts);
        SDL_Rect tr = { (w - ts->w) / 2, 10, ts->w, ts->h};
        SDL_RenderCopy(rnd, tt, NULL, &tr);
    }

    {
        SDL_Surface *ts = TTF_RenderText_Blended(font, "CH1", argb_hex_to_SDL_Color(COLOR_PURPLE));
        SDL_Texture *tt = SDL_CreateTextureFromSurface(rnd, ts);
        SDL_Rect tr = { (w - ts->w) / 2, h/8, ts->w, ts->h};
        SDL_RenderCopy(rnd, tt, NULL, &tr);
    }

    {
        SDL_Surface *ts = TTF_RenderText_Blended(font, "CH2", argb_hex_to_SDL_Color(COLOR_GREEN));
        SDL_Texture *tt = SDL_CreateTextureFromSurface(rnd, ts);
        SDL_Rect tr = { (w - ts->w) / 2, h/4 + h/8, ts->w, ts->h};
        SDL_RenderCopy(rnd, tt, NULL, &tr);
    }

    {
        SDL_Surface *ts = TTF_RenderText_Blended(font, "CH3", argb_hex_to_SDL_Color(COLOR_LIGHT_RED));
        SDL_Texture *tt = SDL_CreateTextureFromSurface(rnd, ts);
        SDL_Rect tr = { (w - ts->w) / 2, 2*h/4 + h/8, ts->w, ts->h};
        SDL_RenderCopy(rnd, tt, NULL, &tr);
    }

        SDL_SetRenderTarget(rnd, NULL);

        SDL_Rect dstrect = {win_x - panel_width, 0, panel_width, h};
        SDL_RenderCopy(rnd, txt, NULL, &dstrect);

}

#if 0
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ts->w, ts->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, ts->pixels);
#endif
