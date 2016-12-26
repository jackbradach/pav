/* File: pav.c
 *
 * Protocol Analyzer Viewer - main entry point.
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
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <omp.h>
#include <time.h>
#include <unistd.h>

#include "pa_usart.h"
#include "cap.h"
#include "saleae.h"
#include "plot.h"

#include "gui/pav_gui.h"

extern void parse_cmdline(int argc, char *argv[], struct pav_opts *opts);

/* Imports an analog capture file, runs it through the decoder, and
 * spits out the results in a table.
 */
void do_usart_decode(struct pav_opts *opts)
{
    pa_usart_ctx_t *usart;
    cap_bundle_t *bun;
    cap_t *cap;

    pa_usart_ctx_init(&usart);
    pa_usart_ctx_map_data(usart, 0);
    pa_usart_set_desc(usart, opts->fin_name);
    saleae_import_analog(opts->fin, &bun);

    cap = cap_bundle_first(bun);
    pa_usart_ctx_set_freq(usart, 1.0f/cap_get_period(cap));
    for (int i = 0; i < opts->loops; i++) {
        pa_usart_decode_chunk(usart, cap);
    }

    pa_usart_fprint_report(opts->fout, usart);

    pa_usart_ctx_cleanup(usart);
}

void do_plot_capture_to_png(struct pav_opts *opts)
{
    cairo_surface_t *cs;
    cap_bundle_t *bun;
    cap_t *cap, *pl_cap;
    plot_t *pl;

    /* Import capture and plot to a Cairo surface*/
    saleae_import_analog(opts->fin, &bun);
    cap = cap_bundle_first(bun);

    /* User requested range from capture. */
    if (opts->range_begin != 0 || opts->range_end != 0) {
        pl_cap = cap_create_subcap(cap, opts->range_begin, opts->range_end);
    } else {
        pl_cap = cap;
    }

    plot_from_cap(pl_cap, -1, &pl);

    cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1024, 768);
    plot_to_cairo_surface(pl, cs);
    cairo_surface_write_to_png(cs, "test.png");
    cairo_surface_destroy(cs);
    cap_dropref(pl_cap);
}


int main(int argc, char *argv[])
{
    struct pav_opts opts;

    /* Make printf add an appropriate thousand's delimiter, based on locale */
    setlocale(LC_NUMERIC, "");

    parse_cmdline(argc, argv, &opts);

    switch (opts.op) {
        case PAV_OP_DECODE:
            do_usart_decode(&opts);
            break;

        case PAV_OP_PLOTPNG:
            do_plot_capture_to_png(&opts);
            break;

        case PAV_OP_GUI:
            pav_gui_start(&opts);
            break;

        case PAV_OP_INVALID:
        default:
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
