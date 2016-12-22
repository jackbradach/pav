#include <assert.h>
#include <errno.h>
#include <fcntl.h>
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
static void inflate_buffer(void **buf, size_t *buf_len);

void saleae_import_analog(FILE *fp, struct cap_bundle **new_bundle)
{
    void *abuf;
    size_t abuf_len;
    struct cap_bundle *bun;
    struct saleae_analog_header *hdr;

    mmap_file(fp, &abuf, &abuf_len);
    inflate_buffer(&abuf, &abuf_len);
    hdr = (struct saleae_analog_header *) abuf;
    assert(hdr->channel_count > 0 && hdr->channel_count <= 16);

    bun = cap_bundle_create();

    for (uint16_t ch = 0; ch < hdr->channel_count; ch++) {
        cap_analog_t *acap = cap_analog_create(hdr->sample_total);
        cap_set_physical_ch((cap_t *) acap, ch);
        cap_set_period((cap_t *) acap, hdr->sample_period);
        import_analog_channel(abuf, ch, acap);
        cap_set_analog_minmax(acap);

        /* Make a digital version of the analog capture */
        adc_acap_ttl(acap);
        cap_bundle_add(bun, (cap_t *) acap);
    }

    /* Unmap the capture file. */
    munmap(abuf, abuf_len);

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
    void *dbuf;
    size_t dbuf_len;
    cap_digital_t *d;
    uint64_t nsamples;
    int rc;

    mmap_file(fp, &dbuf, &dbuf_len);
    inflate_buffer(&dbuf, &dbuf_len);

    nsamples = dbuf_len / sample_width;

    d = cap_digital_create(dbuf_len);
    cap_set_period((cap_t *) d, 1.0f / freq);

    for (uint64_t i = 0; i < nsamples; i++) {
        cap_digital_set_sample(d, i, ((uint32_t *) dbuf)[i]);
    }

    munmap(dbuf, dbuf_len);
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
static void inflate_buffer(void **buf, size_t *buf_len)
{
    size_t dst_len = 4 * (1 << 20); // start with 4 megabytes
    void *dst = malloc(dst_len);
    z_stream strm  = {
        .next_in = (Bytef *) *buf,
        .avail_in = *buf_len,
        .next_out = (Bytef *) dst,
        .avail_out = dst_len,
    };

    int rc;

    /* Note: this bare value is literally what the dang documentation
     * says to put in there for combining window size and whether or not
     * I wants gzip header detection (spointer alert: I does!)
     */
    rc = inflateInit2(&strm, (15 + 32));
    if (Z_OK != rc) {
        return;
    }

    for (int retries = 10; retries > 0; retries--) {
        dst = realloc(dst, dst_len);
        rc = inflate(&strm, Z_FINISH);
        assert(rc != Z_STREAM_ERROR);

        if (Z_STREAM_END == rc) {
            break;
        } else if (Z_BUF_ERROR == rc) {
            /* Bump the decompression buffer size */
            dst_len *= 2;
        } else {
            /* Not compressed! */
            inflateEnd(&strm);
            return;
        }
    }
    *buf = realloc(dst, strm.total_out);
    *buf_len = strm.total_out;
}
