/* File: saleae.h
 *
 * Protocol Analyzer Viewer - capture file import (Saleae format)
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
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "adc.h"
#include "file_utils.h"

struct __attribute__((__packed__)) saleae_analog_header {
    uint64_t sample_total;
    uint32_t channel_count;
    double sample_period;
};

/* Local prototypes */
static void import_analog_channel(void *abuf, unsigned ch, cap_t *cap);

int saleae_import_analog(FILE *fp, struct cap_bundle **new_bundle)
{
    void *abuf;
    size_t abuf_len;
    struct cap_bundle *bun;
    struct saleae_analog_header *hdr;
    int rc;

    rc = file_load(fp, &abuf, &abuf_len);
    if (rc) {
        *new_bundle = NULL;
        errno = EIO;
        return -1;
    }

    /* TODO - better sanity check on header contents */
    hdr = (struct saleae_analog_header *) abuf;
    if (hdr->channel_count > 16) {
        errno = ENODATA;
        return -1;
    }

    bun = cap_bundle_create();

    for (uint16_t ch = 0; ch < hdr->channel_count; ch++) {
        cap_t *cap = cap_create(hdr->sample_total);
        cap_set_physical_ch(cap, ch);
        cap_set_period(cap, hdr->sample_period);
        import_analog_channel(abuf, ch, cap);
        cap_update_analog_minmax(cap);

        /* Make a digital version of the analog capture */
        cap_analog_adc_ttl(cap);
        cap_bundle_add(bun, cap);

        {
            uint16_t min, max, diff;
            min = cap_get_analog_min(cap);
            max = cap_get_analog_max(cap);
            diff = max - min;
            printf("Analog import min/max: %d/%d\n", min, max);
            printf("Analog sample range: %d (%.0f bits)\n",
                diff, floor(log2(diff) + 1));
            printf("Analog resolution: %.2f mV\n",
                1000 * (20.0/4096));

        }
    }

    free(abuf);
    *new_bundle = bun;
    return 0;
}

static void import_analog_channel(void *abuf, unsigned ch, cap_t *cap)
{
    struct saleae_analog_header *hdr = (struct saleae_analog_header *) abuf;
    float *ch_start;

    ch_start = (float *) (abuf + sizeof(struct saleae_analog_header) +
        (ch * (sizeof(float) * hdr->sample_total)));

    /* The loop instead of memcpy is to convert from float
     * to uint16_t.
     */
    for (uint64_t i = 0; i < hdr->sample_total; i++) {
        cap_set_analog(cap, i, (uint16_t) ch_start[i]);
     }
}
