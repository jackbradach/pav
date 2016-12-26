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
static void import_analog_channel(void *abuf, unsigned ch, cap_analog_t *acap);

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
        cap_analog_t *acap = cap_analog_create(hdr->sample_total);
        cap_set_physical_ch((cap_t *) acap, ch);
        cap_set_period((cap_t *) acap, hdr->sample_period);
        import_analog_channel(abuf, ch, acap);
        cap_analog_set_minmax(acap);

        /* Make a digital version of the analog capture */
        cap_analog_adc_ttl(acap);
        cap_bundle_add(bun, (cap_t *) acap);
    }

    free(abuf);
    *new_bundle = bun;
    return 0;
}

static void import_analog_channel(void *abuf, unsigned ch, cap_analog_t *acap)
{
    struct saleae_analog_header *hdr = (struct saleae_analog_header *) abuf;
    float *ch_start;

    ch_start = (float *) (abuf + sizeof(struct saleae_analog_header) +
        (ch * (sizeof(float) * hdr->sample_total)));
    for (uint64_t i = 0; i < hdr->sample_total; i++) {
        cap_analog_set_sample(acap, i, (uint16_t) ch_start[i]);
     }
}

#if 0
// TODO - need to re-evaluate how digital captures are imported...
int saleae_import_digital(FILE *fp, size_t sample_width, float freq, cap_digital_t **dcap)
{
    void *buf;
    size_t buf_len;
    cap_digital_t *d;
    uint64_t nsamples;

    if (file_load(fp, &buf, &buf_len) < 1) {
        *dcap = NULL;
        return -1;
    }

    nsamples = buf_len / sample_width;

    d = cap_digital_create(buf_len);
    cap_set_period((cap_t *) d, 1.0f / freq);

    for (uint64_t i = 0; i < nsamples; i++) {
        cap_digital_set_sample(d, i, ((uint32_t *) buf)[i]);
    }

    free(buf);
    *dcap = d;

    return 0;
}
#endif
