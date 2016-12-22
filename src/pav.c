/* File: pav.c
 *
 * Protocol Analyzer Validation - main entry point.
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

#include "pa_spi.h"
#include "pa_usart.h"
#include "adc.h"
#include "cap.h"
#include "gui.h"
#include "saleae.h"
#include "sstreamer.h"
#include "plot.h"

#include "pav.h"

extern void parse_cmdline(int argc, char *argv[], struct pav_opts *opts);

#if 0
void test_gui(gui_ctx_t *gui)
{
    clock_t ts_start, ts_end;
    double elapsed;
    struct cap_bundle *bun;
    struct cap_analog *acap;
    int rc;

    uint64_t sample_count = 0;
    uint64_t decode_count = 0;

    saleae_import_analog("bin/uart_analog_115200_50mHz.bin", &bun);

    // XXX - Testing!
    acap = (struct cap_analog *) TAILQ_FIRST(bun->caps);

    ts_start = clock();

    plot_analog_cap_gui(gui, acap, 10000, 20000);

    gui_draw(gui);
    ts_end = clock();
    elapsed = (double)(ts_end - ts_start) / CLOCKS_PER_SEC;
    printf("Real Time: %f seconds\n", elapsed);
    cap_bundle_dropref(bun);
}
#endif

void do_usart_decode(struct pav_opts *opts)
{
    // open file w/gzip
    // run through decoder
    // print results.
    char *usart_recv;
    uint64_t decode_cnt;

    pa_usart_ctx_t *usart;
    cap_bundle_t *bun;
    cap_t *cap;

    pa_usart_ctx_init(&usart);
    pa_usart_ctx_map_data(usart, 0);
    pa_usart_set_desc(usart, opts->fin_name);
    saleae_import_analog(opts->fin, &bun);
    pa_usart_ctx_set_freq(usart, 50.0E6);

    cap = cap_bundle_first(bun);
    pa_usart_decode_chunk(usart, cap);

    decode_cnt = pa_usart_get_decoded(usart, &usart_recv);

    pa_usart_fprint_report(opts->fout, usart);
    free(usart_recv);
    pa_usart_ctx_cleanup(usart);
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
    }


    //fclose(opts.fin);
    //fclose(opts.fout);
    return EXIT_SUCCESS;
}
