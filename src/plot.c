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

#define PLOT_LABEL_MAXLEN 64

struct plot_data {
    float *x, *y;
    float xmin, xmax;
    float ymin, ymax;
    unsigned long len;
    char xlabel[PLOT_LABEL_MAXLEN];
    char ylabel[PLOT_LABEL_MAXLEN];
    char title[PLOT_LABEL_MAXLEN];
    struct refcnt rcnt;
};

static void plot_data_free(const struct refcnt *ref);

void plot_data_from_acap(struct cap_analog *acap, struct plot_data **new_plot)
{
    // extract x / y from acaps
    // call plot_data create
}

/* Function: plot_data_create
 *
 * Allocates and initializes a new plot structure
 *
 * Parameters:
 *  dcap - pointer to uninitialized handle for an digital capture structure
 *
 * See Also:
 *  <plot_data_destroy>, <plot_data_set_title>
 */
void plot_data_create(struct plot_data **new_plot)
{
    struct plot_data *p;

    p = calloc(1, sizeof(struct plot_data));
    p->rcnt = (struct refcnt) { plot_data_free, 1 };

    *new_plot = p;
}

/* Function: plot_data_destroy
 *
 * Mark an plot data structure as no longer used; if no otherwise
 * references are held to the structure, it'll be cleaned up and the
 * memory freed.
 *
 * Parameters:
 *  p - handle to an plot structure
 */
void plot_data_destroy(struct plot_data *p)
{
    assert(NULL != p);
    refcnt_dec(&p->rcnt);
}

static void plot_data_free(const struct refcnt *ref)
{
    struct plot_data *p =
        container_of(ref, struct plot_data, rcnt);

    assert(NULL != p);

    if (p->x)
        free(p->x);

    if (p->y)
        free(p->y);

    free(p);
}

void plot_data_set_xlabel(struct plot_data *p, const char *xlabel)
{
    snprintf(p->xlabel, sizeof(p->xlabel), "%s", xlabel);
}

void plot_data_set_ylabel(struct plot_data *p, const char *ylabel)
{
    snprintf(p->ylabel, sizeof(p->ylabel), "%s", ylabel);
}

void plot_data_set_title(struct plot_data *p, const char *title)
{
    snprintf(p->title, sizeof(p->title), "%s", title);
}

const char *plot_data_get_ylabel(struct plot_data *p)
{
    return p->ylabel;
}

const char *plot_data_get_xlabel(struct plot_data *p)
{
    return p->xlabel;
}

const char *plot_data_get_title(struct plot_data *p)
{
    return p->title;
}

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
