#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "cap.h"
#include "gui.h"
#include "refcnt.h"

#include <SDL2/SDL.h>

// PLPLOT
#include "plplot/plplot.h"
#include "cairo/cairo.h"

#include "plot.h"

#define PLOT_LABEL_MAXLEN 64

struct plot {
    double *x, *y;
    double ymin, ymax;
    unsigned long len;
    char xlabel[PLOT_LABEL_MAXLEN];
    char ylabel[PLOT_LABEL_MAXLEN];
    char title[PLOT_LABEL_MAXLEN];
    struct refcnt rcnt;
};

static void plot_free(const struct refcnt *ref);
static void plot_from_acap(struct cap_analog *acap, uint64_t idx_begin, uint64_t idx_end, struct plot **plot);

void plot_from_cap(cap_t *cap, int64_t idx_begin, uint64_t idx_end, struct plot **plot)
{
    plot_from_acap((struct cap_analog *) cap, idx_begin, idx_end, plot);
}

static void plot_from_acap(struct cap_analog *acap, uint64_t idx_begin, uint64_t idx_end, struct plot **plot)
{
    struct plot *p;
    uint16_t smin, smax;
    adc_cal_t *cal;

    p = plot_create();

    /* Set the y-axis scales to the voltage range */
    smin = cap_analog_get_sample_min(acap);
    smax = cap_analog_get_sample_max(acap);
    cal = cap_analog_get_cal(acap);

    p->ymin = adc_sample_to_voltage(smin, cal);
    p->ymax = adc_sample_to_voltage(smax, cal);
    p->len = idx_end - idx_begin;

    p->x = calloc(p->len, sizeof(double));
    p->y = calloc(p->len, sizeof(double));

    // extract x / y from acaps
    for (uint64_t i = idx_begin, j = 0; i < idx_end; i++, j++) {
        uint16_t sample = cap_analog_get_sample(acap, i);
        float v = adc_sample_to_voltage(sample, cal);
        // FIXME: the array needs to start at zero even if the indices do not!
        p->x[j] = (idx_begin + j);
        p->y[j] = v;
    }

    // call plot create
    *plot = p;
}

void plot_to_wxwidgets(struct plot *p)
{
    plsdev("wxwidgets");
    plinit();
    plenv(p->x[0], p->x[p->len - 1], p->ymin, p->ymax + (p->ymax / 10), 0, 0);
    pllab(p->xlabel, p->ylabel, p->title);
    plcol0(3);
    plline(p->len, (PLFLT *) p->x, (PLFLT *) p->y);
    plend();
}

void plot_to_cairo_surface(struct plot *p, cairo_surface_t *cs)
{
    cairo_t *c;
    int w, h;

    c = cairo_create(cs);
    w = cairo_image_surface_get_width(cs);
    h = cairo_image_surface_get_height(cs);

    cairo_scale(c, w, h);
    cairo_set_source_rgba(c, 1, 0.2, 0.2, 0.6);
    cairo_fill(c);

    plsdev("extcairo");
    plinit();
    pl_cmd(PLESC_DEVINIT, c);
    plenv(p->x[0], p->x[p->len - 1], p->ymin, p->ymax + (p->ymax / 10), 0, 0);
    pllab(p->xlabel, p->ylabel, p->title);
    plcol0(3);
    plline(p->len, (PLFLT *) p->x, (PLFLT *) p->y);
    plend();
    cairo_destroy(c);
}

/* Function: plot_create
 *
 * Allocates and initializes a new plot structure
 *
 */
struct plot *plot_create(void)
{
    struct plot *p;

    p = calloc(1, sizeof(struct plot));
    p->rcnt = (struct refcnt) { plot_free, 1 };

    return p;
}

/* Function: plot_addref
 *
 * Adds a reference to a plot_t, returning a pointer to the caller.
 *
 * Parameters:
 *  p - existing plot_t structure
 *
 * Returns:
 *  pointer to capture structure for caller
 *
 * See Also:
 *  <plot_dropref>
 */
struct plot *plot_addref(struct plot *p)
{
    if (NULL == p)
        return NULL;

    refcnt_inc(&p->rcnt);
    return p;
}

/* Function: plot_dropref
 *
 * Mark an plot data structure as no longer used; if no otherwise
 * references are held to the structure, it'll be cleaned up and the
 * memory freed.
 *
 * Parameters:
 *  p - handle to an plot structure
 */
void plot_dropref(struct plot *p)
{
    if (NULL == p)
        return;
    refcnt_dec(&p->rcnt);
}

/* Function: plot_getref
 *
 * Returns the number of active references on a plot_t
 *
 * Parameters:
 *  p - pointer to a plot_t
 *
 * Returns:
 *  Number of active references
 *
 * See Also:
 *  <plot_addref>, <plot_dropref>
 */
unsigned plot_getref(struct plot *p)
{
    if (NULL == p)
        return 0;
    return p->rcnt.count;
}

static void plot_free(const struct refcnt *ref)
{
    struct plot *p =
        container_of(ref, struct plot, rcnt);

    if (p->x)
        free(p->x);

    if (p->y)
        free(p->y);

    free(p);
}

void plot_set_xlabel(struct plot *p, const char *xlabel)
{
    snprintf(p->xlabel, sizeof(p->xlabel), "%s", xlabel);
}

void plot_set_ylabel(struct plot *p, const char *ylabel)
{
    snprintf(p->ylabel, sizeof(p->ylabel), "%s", ylabel);
}

void plot_set_title(struct plot *p, const char *title)
{
    snprintf(p->title, sizeof(p->title), "%s", title);
}

const char *plot_get_ylabel(struct plot *p)
{
    return p->ylabel;
}

const char *plot_get_xlabel(struct plot *p)
{
    return p->xlabel;
}

const char *plot_get_title(struct plot *p)
{
    return p->title;
}


#if 0
void plot_analog_cap_sdl(SDL_Texture *texture, struct cap_analog *acap, unsigned idx_start, unsigned idx_end);
void plot_analog_cap_cairo(cairo_surface_t *surface, struct cap_analog *acap, unsigned idx_start, unsigned idx_end);

void plot_analog_cap(struct cap_analog *acap, unsigned idx_start, unsigned idx_end)
{
    float vmin, vmax;
    uint16_t min, max;
    PLFLT *x, *y;
    PLFLT xmin, ymin, xmax, ymax;

    vmin = adc_sample_to_voltage(acap->sample_min, acap->cal);
    vmax = adc_sample_to_voltage(acap->sample_max, acap->cal);
    printf("Min: %d (%02fV)\n", min, vmin);
    printf("Max: %d (%02fV)\n", max, vmax);

    x = calloc(acap->nsamples, sizeof(PLFLT));
    y = calloc(acap->nsamples, sizeof(PLFLT));

    xmin = 0;
    ymin = vmin;
    xmax = (idx_end - idx_start);
    ymax = vmax;

    for (uint64_t i = idx_start, j = 0; i < idx_end; i++, j++) {
        // FIXME: the array needs to start at zero even if the indices do not!
        x[j] = (PLFLT) (idx_start + j);
        y[j] = adc_sample_to_voltage(acap->samples[i], acap->cal);
    }

    plsdev("wxwidgets");
    plinit();
    plenv(idx_start, idx_end, ymin, ymax + (ymax / 10), 0, 0);
    pllab("sample", "Voltage", "Analog Sample");
    plcol0(3);
    plline((idx_end - idx_start), x, y);
    plend();
    free(x);
    free(y);
}
#endif


#if 0

void plot_analog_cap_gui(struct gui_ctx *gui, struct cap_analog *acap, unsigned idx_start, unsigned idx_end)
{
    plot_analog_cap_sdl(gui_get_texture(gui), acap, idx_start, idx_end);
}


void plot_analog_cap_sdltex(struct cap_analog *acap, unsigned idx_start, unsigned idx_end, SDL_Texture **texture)
{
    int w, h;
    SDL_Surface *ss;
    cairo_surface_t *cs;

    SDL_QueryTexture(texture, NULL, NULL, &w, &h);

    ss = SDL_CreateRGBSurface (
        0, w, h, 32,
        0x000000FF, /* Red channel mask */
        0x0000FF00, /* Green channel mask */
        0x00FF0000, /* Blue channel mask */
        0xFF000000); /* Alpha channel mask */


    SDL_LockTexture(texture, NULL, &ss->pixels, &ss->pitch);
    cs = cairo_image_surface_create_for_data(
            ss->pixels, CAIRO_FORMAT_ARGB32,
            ss->w, ss->h, ss->pitch);

    assert(cairo_surface_status(cs) == CAIRO_STATUS_SUCCESS);
    cairo_surface_set_user_data(cs, &CAIRO_SURFACE_TARGET_KEY, ss, NULL );

    //plot_analog_cap_cairo(cs, acap, idx_start, idx_end);
    cairo_t *cairo;
    cairo = cairo_create(cs);
    cairo_scale(cairo, w, h);
    cairo_set_source_rgba (cairo, 1, 0.2, 0.2, 0.6);
    cairo_fill(cairo);

    cairo_surface_flush(cs);
    cairo_surface_destroy(cs);
    SDL_UnlockTexture(texture);
    cairo_destroy(cairo);
}

void plot_analog_cap_cairo(cairo_surface_t *surface, struct cap_analog *acap, unsigned idx_start, unsigned idx_end)
{
    float vmin, vmax;
    uint16_t min, max;
    PLFLT *x, *y;
    PLFLT xmin, ymin, xmax, ymax;
    int w, h;

    cairo_t *cairo;

    w = cairo_image_surface_get_width(surface);
    h = cairo_image_surface_get_height(surface);

    cairo = cairo_create(surface);
    cairo_scale(cairo, w, h);
    cairo_set_source_rgba (cairo, 1, 0.2, 0.2, 0.6);
    cairo_fill(cairo);
    return;
    //    draw (cairo_ctx);


    // TODO: Should have a function that preps a "plot" structure
    // TODO: with enough info to easily switch backends.
    vmin = adc_sample_to_voltage(acap->sample_min, acap->cal);
    vmax = adc_sample_to_voltage(acap->sample_max, acap->cal);

    x = calloc(acap->nsamples, sizeof(PLFLT));
    y = calloc(acap->nsamples, sizeof(PLFLT));

    xmin = 0;
    ymin = vmin;
    xmax = (idx_end - idx_start);
    ymax = vmax;

    for (uint64_t i = idx_start, j = 0; i < idx_end; i++, j++) {
        x[j] = (PLFLT) (idx_start + j);
        y[j] = adc_sample_to_voltage(acap->samples[i], acap->cal);
    }

    plsdev("extcairo");
    plinit();
    pl_cmd(PLESC_DEVINIT, cairo);
    plenv(idx_start, idx_end, ymin, ymax + (ymax / 10), 0, 0);
    pllab("sample", "Voltage", "Analog Sample");
    plcol0(3);
    plline((idx_end - idx_start), x, y);
    plend();

    free(x);
    free(y);
    //cairo_destroy(cairo);
}

#endif
