#ifndef _VIEWS_H_
#define _VIEWS_H_

#include <stdint.h>

#include "cap.h"
#include "plot.h"
#include "gui.h"
#include "queue.h"
#include "shaders.h"

enum view_flags {
    VIEW_CLEAN = 0x0,
    VIEW_PLOT_DIRTY = 0x1,
    VIEW_TEXTURE_DIRTY = 0x2
};

struct ch_view {
    TAILQ_ENTRY(ch_view) entry;
    cap_t *cap;
    plot_t *pl;
    SDL_Texture *txt;
    shader_t *shader;
    uint64_t sample_min;
    uint64_t sample_max;
    uint64_t sample_selected;
    unsigned zoom_level;
    uint32_t flags;
};
TAILQ_HEAD(ch_view_list, ch_view);

#ifdef __cplusplus
extern "C" {
#endif

void views_populate_from_bundle(cap_bundle_t *b, struct ch_view_list **v);
void views_create_list(struct ch_view_list **l);
void views_add_ch(struct ch_view_list *v, cap_t *c);
void views_refresh(struct ch_view *v);

void views_zoom_in(struct ch_view *v);
void views_zoom_out(struct ch_view *v);
void views_pan_left(struct ch_view *v);
void views_pan_right(struct ch_view *v);

#ifdef __cplusplus
}
#endif

#endif
