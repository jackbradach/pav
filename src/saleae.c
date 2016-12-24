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
#include <sys/mman.h>
#include <unistd.h>

#include <zlib.h>

#include "adc.h"

struct __attribute__((__packed__)) saleae_analog_header {
    uint64_t sample_total;
    uint32_t channel_count;
    double sample_period;
};

/* Local prototypes */
static void mmap_file(FILE *fp, void **buf, size_t *length);
static void import_analog_channel(void *abuf, unsigned ch, cap_analog_t *acap);
static bool inflate_buffer(void *src, size_t src_len, void **dst, size_t *dst_len);

void saleae_import_analog(FILE *fp, struct cap_bundle **new_bundle)
{
    void *fbuf, *abuf;
    size_t fbuf_len, abuf_len;
    struct cap_bundle *bun;
    struct saleae_analog_header *hdr;
    bool compressed;

    mmap_file(fp, &fbuf, &fbuf_len);
    compressed = inflate_buffer(fbuf, fbuf_len, &abuf, &abuf_len);
    hdr = (struct saleae_analog_header *) abuf;
    assert(hdr->channel_count > 0 && hdr->channel_count <= 16);

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

    /* Unmap the capture file and free the memory allocated if it
     * was compressed.
     */
    munmap(fbuf, fbuf_len);
    if (compressed) {
        free(abuf);
    }

    *new_bundle = bun;
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

int saleae_import_digital(FILE *fp, size_t sample_width, float freq, cap_digital_t **dcap)
{
    void *fbuf, *dbuf;
    size_t fbuf_len, dbuf_len;
    cap_digital_t *d;
    uint64_t nsamples;
    bool compressed;

    mmap_file(fp, &fbuf, &fbuf_len);
    compressed = inflate_buffer(fbuf, fbuf_len, &dbuf, &dbuf_len);

    nsamples = dbuf_len / sample_width;

    d = cap_digital_create(dbuf_len);
    cap_set_period((cap_t *) d, 1.0f / freq);

    for (uint64_t i = 0; i < nsamples; i++) {
        cap_digital_set_sample(d, i, ((uint32_t *) dbuf)[i]);
    }

    munmap(fbuf, fbuf_len);
    if (compressed) {
        free(dbuf);
    }

    *dcap = d;

    return 0;
}

/* Function: mmap_file
 *
 * Maps a file contents into memory.
 *
 */
static void mmap_file(FILE *fp, void **buf, size_t *len)
{
    struct stat st;
    void *addr;

    fstat(fileno(fp), &st);
    addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fileno(fp), 0);
    *len = st.st_size;
    *buf = addr;
}

/* Attempts to decompress a memmaped gzip file in-place.  If the buffer
 * doesn't contain a GZIP image, it'll be left untouched.  Otherwise,
 * buf and buf_len will be replaced with the decompressed image.
 */
static bool inflate_buffer(void *src, size_t src_len, void **dst, size_t *dst_len)
{
    size_t buf_len = 4 * (1 << 20); // start with 4 megabytes
    void *buf = calloc(buf_len, sizeof(uint8_t));
    z_stream strm  = {
        .next_in = (Bytef *) src,
        .avail_in = src_len,
    };

    int rc;

    /* Note: this bare value is literally what the dang documentation
     * says to put in there for combining window size and whether or not
     * I wants gzip header detection (spointer alert: I does!)
     */
    rc = inflateInit2(&strm, (15 + 32));
    if (Z_OK != rc) {
        return false;
    }

    for (int retries = 10; retries > 0; retries--) {
        buf = realloc(buf, buf_len);
        strm.next_out = (Bytef *) buf;
        strm.avail_out = buf_len;
        rc = inflate(&strm, Z_FINISH);
        assert(rc != Z_STREAM_ERROR);

        if (Z_STREAM_END == rc) {
            break;
        } else if (Z_BUF_ERROR == rc) {
            /* Bump the decompression buffer size */
            buf_len *= 2;
        } else {
            /* Not compressed! */
            inflateEnd(&strm);
            free(buf);
            *dst = src;
            *dst_len = src_len;
            return false;
        }
    }

    *dst = realloc(buf, strm.total_out);
    *dst_len = strm.total_out;
    return true;
}
