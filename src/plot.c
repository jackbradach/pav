/* File: plot.c
 *
 * Protocol Analyzer Viewer - plotting routines
 *
 * Author: Jack Bradach <jack@bradach.net>
 *
 * Copyright (C) 2016 Jack Bradach <jack@bradach.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cap.h"
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
    int64_t reticle;
    char xlabel[PLOT_LABEL_MAXLEN];
    char ylabel[PLOT_LABEL_MAXLEN];
    char title[PLOT_LABEL_MAXLEN];
    struct refcnt rcnt;
};

static void plot_free(const struct refcnt *ref);
static void plot_from_acap(struct cap_analog *acap, struct plot **plot);
static void sprint_plot_cap_title(cap_t *cap, char *s);

/* Function: plot_from_cap
 *
 * Creates a plot from a capture structure
 *
 */
void plot_from_cap(cap_t *cap, struct plot **plot)
{
    plot_from_acap((struct cap_analog *) cap, plot);
}

static void plot_from_acap(struct cap_analog *acap, struct plot **plot)
{
    struct plot *pl;
    char str_label[PLOT_LABEL_MAXLEN];
    uint16_t smin, smax;
    uint64_t offset;
    uint64_t nsamples;
    adc_cal_t *cal;

    pl = plot_create();

    /* Set the y-axis scales to the voltage range */
    smin = cap_analog_get_sample_min(acap);
    smax = cap_analog_get_sample_max(acap);
    cal = cap_analog_get_cal(acap);
    pl->ymin = adc_sample_to_voltage(smin, cal);
    pl->ymax = adc_sample_to_voltage(smax, cal);

    nsamples = cap_get_nsamples((cap_t *) acap);
    offset = cap_get_offset((cap_t *) acap);
    pl->x = calloc(nsamples, sizeof(double));
    pl->y = calloc(nsamples, sizeof(double));
    pl->len = nsamples;

    for (uint64_t i = 0, j = offset; i < nsamples; i++, j++) {
        uint16_t sample = cap_analog_get_sample(acap, i);
        float v = adc_sample_to_voltage(sample, cal);
        pl->x[i] = j;
        pl->y[i] = v;
    }

    snprintf(str_label, PLOT_LABEL_MAXLEN, "Sample @ %'lu = %.02f V",
        (nsamples / 2) + offset, pl->y[nsamples/2]);
    plot_set_xlabel(pl, str_label);
    plot_set_ylabel(pl, "Volts");
    sprint_plot_cap_title((cap_t *) acap, pl->title);

    *plot = pl;
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

void plot_to_texture(struct plot *pl, SDL_Texture *txt)
{
    cairo_surface_t *cs;
    void *pixels;
    int pitch;
    int w, h;

    SDL_QueryTexture(txt, NULL, NULL, &w, &h);
    SDL_LockTexture(txt, NULL, &pixels, &pitch);
    cs = cairo_image_surface_create_for_data(
            pixels, CAIRO_FORMAT_ARGB32, w, h, pitch);
    plot_to_cairo_surface(pl, cs);
    cairo_surface_finish(cs);
    cairo_surface_destroy(cs);
    SDL_UnlockTexture(txt);
}

void plot_to_cairo_surface(struct plot *pl, cairo_surface_t *cs)
{
    cairo_t *c;
    int w, h;
    char res_str[] = "XXXXxYYYY";
    PLFLT x_ret = pl->x[pl->reticle];

    c = cairo_create(cs);
    w = cairo_image_surface_get_width(cs);
    h = cairo_image_surface_get_height(cs);

    //cairo_scale(c, 1.0/w, 1.0/h);
    cairo_set_source_rgba(c, 0, 0, 0, 1.0);
    cairo_fill(c);
    cairo_paint(c);

    sprintf(res_str, "%dx%d", w, h);
    plsdev("extcairo");
    plsetopt("geometry", res_str);
    plinit();
    pl_cmd(PLESC_DEVINIT, c);
    plenv(pl->x[0], pl->x[pl->len - 1], pl->ymin, pl->ymax + (pl->ymax / 10), 0, 0);
    plcol0(2);
    pllab(pl->xlabel, pl->ylabel, pl->title);
    plcol0(3);
    plline(pl->len, (PLFLT *) pl->x, (PLFLT *) pl->y);

    /* Draw reticle */
    plcol0(12);
    pljoin(x_ret, pl->ymin, x_ret, 2 * pl->ymax);

    plstring(1, &x_ret, &pl->y[pl->reticle], "X");

    plend();
    cairo_surface_flush(cs);
    cairo_destroy(c);
}

/* Extracts a plot header from a cap_t. */
static void sprint_plot_cap_title(cap_t *cap, char *s)
{
    uint64_t nsamples = cap_get_nsamples(cap);
    float period = cap_get_period(cap);
    double sampfreq = 1.0 / period;
    double sampfreq_scaled = sampfreq / 1E6;
    const char unit_ratemega[] = "MS/s";

    double duration = nsamples * period;
    double duration_scaled;
    const char tbase_s[] = "s";
    const char tbase_ms[] = "ms";
    const char tbase_us[] = "us";
    const char tbase_ns[] = "ns";
    const char *tbase_unit = tbase_s;

    /* If we can we want to scale the duration timebase from seconds to
     * something easier to think about (eg, no scientific notation)
     */
    if (duration < 1E-6) {
        tbase_unit = tbase_ns;
        duration_scaled = duration * 1E9;
    } else if (duration < 1E-3) {
        tbase_unit = tbase_us;
        duration_scaled = duration * 1E6;
    } else if (duration < 1) {
        tbase_unit = tbase_ms;
        duration_scaled = duration * 1E3;
    }

    snprintf(s, PLOT_LABEL_MAXLEN, "%'lu samples @ %.02f %s (%.2f %s)",
        nsamples, sampfreq_scaled, unit_ratemega, duration_scaled, tbase_unit);
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

void plot_set_reticle(struct plot *p, int64_t idx)
{
    p->reticle = idx;
}

int64_t plot_get_reticle(struct plot *p)
{
    return p->reticle;
}
