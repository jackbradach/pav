#include <GL/glew.h>
#include <GL/glut.h>

#include "cap.h"
#include "gui.h"
#include "views.h"

struct view {
    TAILQ_ENTRY(view) entry;

    cap_t *cap;
    plot_t *pl;

    uint64_t begin;
    uint64_t end;
    int64_t target;
    float zoom;
    uint32_t flags;

    char *glyph;

    /* Parameters which affect rendering */
    GLuint vbo_vertices;
    GLuint vbo_idx;
    shader_t *shader;
    float line_width;
    struct {
        float red;
        float green;
        float blue;
        float alpha;
    } color;

    SDL_Texture *txt; // Drop or use for background?

};
TAILQ_HEAD(view_list, view);

struct views {
    struct view_list head;
    unsigned len;
};

void views_add_ch(struct views *vl, cap_t *c);
struct view *view_from_ch(cap_t *c);

void views_create_list(struct views **vl)
{
    struct views *views;

    views = calloc(1, sizeof(struct views));
    TAILQ_INIT(&views->head);
    *vl = views;
}

cap_t *views_get_cap(view_t *v)
{
    return v->cap;
}

SDL_Texture *views_get_texture(view_t *v)
{
    return v->txt;
}

GLuint views_get_vbo_idx(view_t *v)
{
    return v->vbo_idx;
}

GLuint views_get_vbo_vertices(view_t *v)
{
    return v->vbo_vertices;
}

float views_get_line_width(view_t *v)
{
    return v->line_width;
}

float views_get_red(view_t *v)
{
    return v->color.red;
}

float views_get_green(view_t *v)
{
    return v->color.green;
}

float views_get_blue(view_t *v)
{
    return v->color.blue;
}

view_t *views_first(views_t *vl)
{
    return TAILQ_FIRST(&vl->head);
}

view_t *views_next(view_t *v)
{
    return TAILQ_NEXT(v, entry);
}

view_t *views_prev(view_t *v)
{
    return TAILQ_PREV(v, view_list, entry);
}

view_t *views_last(views_t *vl)
{
    return TAILQ_LAST(&vl->head, view_list);
}

char *views_get_glyph(view_t *v)
{
    return v->glyph;
}

unsigned long views_get_begin(view_t *v)
{
    return v->begin;
}

unsigned long views_get_end(view_t *v)
{
    return v->end;
}

unsigned long views_get_width(view_t *v)
{
    return(v->end - v->begin);
}

unsigned views_get_count(views_t *vl)
{
    return vl->len;
}

float views_get_zoom(view_t *v)
{
    return v->zoom;
}

void views_set_range(view_t *v, int64_t begin, int64_t end)
{
    cap_t *c = views_get_cap(v);
    uint64_t nsamples = cap_get_nsamples(c);

    if (begin > 0) {
        v->begin = begin;
    } else {
        v->begin = 0;
    }

    if ((end > 0) && (end < nsamples)) {
        v->end = end;
    } else {
        v->end = nsamples;
    }
}

int64_t views_get_target(view_t *v)
{
    return v->target;
}

void views_set_target(view_t *v, int64_t n)
{
    v->target = n;
}

void views_populate_from_bundle(cap_bundle_t *b, struct views **vl)
{
    struct gui *g = gui_get_instance();
    unsigned len = cap_bundle_len(b);
    struct views *views;
    int i = 0;
    cap_t *c;

    views_create_list(&views);

    c = cap_bundle_first(b);
    while (NULL != c) {
        struct view *v = view_from_ch(c);
        // XXX - temporary fuckery for testing
        v->color.red = i;
        v->color.green = 1;
        v->color.blue = 1;
        i++;

        if (v->txt) {
            SDL_DestroyTexture(v->txt);
        }
        v->txt= SDL_CreateTexture(g->renderer,
                    SDL_PIXELFORMAT_ARGB8888,
                    SDL_TEXTUREACCESS_STREAMING,
                    GUI_WIDTH, GUI_HEIGHT / len);
        TAILQ_INSERT_TAIL(&views->head, v, entry);
        views->len++;
        c = cap_next(c);

    }

    *vl = views;
}

struct view *view_from_ch(cap_t *c)
{
    struct view *v;

    v = calloc(1, sizeof(struct view));
    v->cap = cap_addref(c);
    v->target = cap_get_nsamples(c) / 2;
    v->begin = 0;
    v->end = cap_get_nsamples(c);
    v->zoom = 1;
    v->flags |= VIEW_PLOT_DIRTY; // XXX - probably drop this
    v->flags |= VIEW_VBO_DIRTY;



    return v;
}


void views_to_vertices(view_t *v, float **vertices)
{
    float *points;
    int idx, i;

    printf("vmin/max = %.02f/%.02f\n", cap_get_analog_vmin(v->cap),
        cap_get_analog_vmax(v->cap));
    points = calloc(2 * views_get_width(v), sizeof(float));
    for (i = 0, idx = v->begin; idx < v->end; idx++, i++) {
        points[(2 * i)] = (float) i / (float) views_get_width(v);
        points[(2 * i) + 1] = cap_get_analog_voltage(v->cap, i);
    }

    *vertices = points;
}

void views_destroy(struct views *vl)
{
    struct view *cur, *tmp;

    TAILQ_FOREACH_SAFE(cur, &vl->head, entry, tmp) {
        TAILQ_REMOVE(&vl->head, cur, entry);
        cap_dropref(cur->cap);
        plot_dropref(cur->pl);
        free(cur);
    }
    free(vl);
}

void views_refresh(struct view *v)
{
    /* Refresh the plot if needed (eg, zoom / pan) */
    if (v->flags & VIEW_PLOT_DIRTY) {
        plot_from_view(v, &v->pl);
        v->flags &= ~VIEW_PLOT_DIRTY;
        v->flags |= VIEW_VBO_DIRTY;
    }

    /* Refesh the texture if dirty (eg, the size changed) */
    if (v->flags & VIEW_TEXTURE_DIRTY) {
        plot_to_texture(v->pl, v->txt);
        v->flags &= ~VIEW_TEXTURE_DIRTY;
    }

    /* Refresh the VBO */
    if (v->flags & VIEW_VBO_DIRTY) {
        GLsizeiptr len_vertices = 2 * cap_get_nsamples(v->cap) * sizeof(float);
        GLsizeiptr len_idx = views_get_width(v) * sizeof(unsigned);
        unsigned *idx = calloc(views_get_width(v), sizeof(unsigned));
        float *points;

        views_to_vertices(v, &points);

        /* Bind vertices to a VBO; existing buffers look to get
         * auto-cleaned when a different binding is set.
         */
        // FIXME: this only needs to happen when the cap is loaded!
        glGenBuffers(1, &v->vbo_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, v->vbo_vertices);
        glBufferData(GL_ARRAY_BUFFER, len_vertices, points, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        /* Regenerate the indices to match the view range */
        for (unsigned i = v->begin, j = 0; i < v->end; i++, j++) {
            idx[j] = i;
        }
        glGenBuffers(1, &v->vbo_idx);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, v->vbo_idx);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, len_idx * sizeof(unsigned), idx, GL_STATIC_DRAW);

        v->flags &= ~VIEW_VBO_DIRTY;
    }
}

static void views_zoom(struct view *v, float level);
void views_zoom_in(struct view *v)
{
    if (v->zoom * 2 >= 4096) {
        printf("Can't zoom in further\n");
        v->zoom = 4096;
    } else {
        v->zoom *= 2;
    }
    views_zoom(v, v->zoom);
}

void views_zoom_out(struct view *v)
{
    if ((v->zoom / 2) <= 1) {
        printf("Can't zoom out further\n");
        v->zoom = 1.0;
    } else {
        v->zoom /= 2;
    }
    views_zoom(v, v->zoom);
}

static void views_zoom(struct view *v, float level)
{
    int64_t z0, z1;
    uint64_t nsamples;
    uint64_t znsamples;

    nsamples = cap_get_nsamples(v->cap);

    znsamples = nsamples / level;
    z0 = v->target - (znsamples / 2);
    z1 = v->target + (znsamples / 2);

    if (z0 < 0) {
        z0 = 0;
    }

    if (z1 >= nsamples) {
        z1 = (nsamples);
    }

    // TODO - critical section
    v->begin = z0;
    v->end = z1;
    v->zoom = level;
    v->flags |= VIEW_PLOT_DIRTY;
}

void views_pan_left(struct view *v)
{
    uint64_t nsamples = cap_get_nsamples(v->cap);
    int64_t prev = cap_prev_edge(v->cap, v->target);

    /* If the previous edge is near the boundary of the capture, snap-scroll
     * to the previous window.
     */
    if ((prev <= v->begin) && (prev > 0)) {
        double begin = v->begin - (views_get_width(v) * .25);
        double end = v->end - (views_get_width(v) * .25);

        if (begin < 0) {
            begin = 0;
        }

        if (end > nsamples) {
            end = nsamples;
        }

        v->begin = begin;
        v->end = end;
    }

    /* Don't lock the reticle onto the first sample as an edge. */
    if (prev > 0) {
        v->target = prev;
    }

    v->flags |= VIEW_PLOT_DIRTY;
}

void views_pan_right(struct view *v)
{
    uint64_t nsamples = cap_get_nsamples(v->cap);
    uint64_t next = cap_next_edge(v->cap, v->target);

    /* If the next edge is near the boundary of the capture, snap-scroll
     * to the next window.
     */
    if ((next > v->end) && (next < nsamples)) {
        uint64_t begin = next - (views_get_width(v) / 2);
        uint64_t end = next + (views_get_width(v) / 2);

        if (begin < 0) {
            begin = 0;
        }

        if (end > nsamples) {
            end = nsamples;
        }

        v->begin = begin;
        v->end = end;
    }

    /* Don't lock the reticle onto the last sample as an edge. */
    if (next < nsamples) {
        v->target = next;
    }

    v->flags |= VIEW_PLOT_DIRTY;
}


void view_draw_gl(struct view *v)
{
    cap_t *c = v->cap;
    double nsamples = cap_get_nsamples(c);
    glLineWidth(2.5);
    glColor3f(1.0, 0.0, 0.0);
    glEnd();
}
