#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include "adc.h"

struct __attribute__((__packed__)) saleae_analog_header {
    uint64_t sample_total;
    uint32_t channel_count;
    double sample_period;
};

/* Local prototypes */
static int mmap_file(const char *filename, void **buf, size_t *length);
static void mmap_file_new(FILE *fp, void **buf, size_t *length);
static void import_analog_channel(void *abuf, unsigned ch, cap_analog_t *acap);

void saleae_import_analog_new(FILE *fp, struct cap_bundle **new_bundle)
{
    void *abuf;
    size_t abuf_len;
    struct cap_bundle *bun;
    struct saleae_analog_header *hdr;

    mmap_file_new(fp, &abuf, &abuf_len);

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

void saleae_import_analog(const char *src_file, struct cap_bundle **new_bundle)
{
    void *abuf;
    size_t abuf_len;
    struct cap_bundle *bun;
    struct saleae_analog_header *hdr;
    int rc;

    rc = mmap_file(src_file, &abuf, &abuf_len);
    if (rc == -ENOENT) {
        printf("Unable to open analog capture file '%s'!\n", src_file);
        // TODO - 2016/12/15 - jbradach - graceful user input handling!
        abort();
    }

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

int saleae_import_digital(const char *cap_file, size_t sample_width, float freq, cap_digital_t **dcap)
{
    void *dbuf;
    size_t dbuf_len;
    cap_digital_t *d;
    uint64_t nsamples;
    int rc;

    rc = mmap_file(cap_file, &dbuf, &dbuf_len);
    if (rc == -ENOENT) {
        printf("Unable to open digital capture file '%s'!\n", cap_file);
        return -1;
    }

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
 * Parameters:
 *      filename - file to load
 *      buf - uninitialized handle which will receive a pointer
 *      len - size_t point in which the length is returned.
 */
static int mmap_file(const char *filename, void **buf, size_t *len)
{
    int fd;
    struct stat st;
    void *addr;

    // TODO - 2016/12/08 - jbradach - add more robust error handling here.
    fd = open(filename, O_RDONLY);
    if (fd <= 0)
        return -ENOENT;

    fstat(fd, &st);

    addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    *len = st.st_size;
    *buf = addr;

    return 0;
}

static void mmap_file_new(FILE *fp, void **buf, size_t *len)
{
    struct stat st;
    void *addr;

    fstat(fileno(fp), &st);
    addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fileno(fp), 0);
    *len = st.st_size;
    *buf = addr;
}
