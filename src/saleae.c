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
static void saleae_print_analog_header(struct saleae_analog_header *hdr);
static void cap_import_channel(void *abuf, unsigned ch, uint16_t *samples_out);

static void saleae_print_analog_header(struct saleae_analog_header *hdr)
{
    printf("Saleae Analog Capture File\n");
    printf("--------------------------\n");
    printf("Total samples: %lu\n", hdr->sample_total);
    printf("Channel count: %u\n", hdr->channel_count);
    printf("Sample period: %.02e\n", hdr->sample_period);
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
    saleae_print_analog_header(hdr);

    assert(hdr->channel_count > 0 && hdr->channel_count <= 16);

    bun = cap_bundle_create();

    for (uint16_t ch = 0; ch < hdr->channel_count; ch++) {
        struct cap_analog *acap = cap_analog_create();
        struct cap_base *cap = &acap->super;
        cap->nsamples = hdr->sample_total;
        cap->physical_ch = ch;
        cap->period = hdr->sample_period;
        acap->samples = calloc(hdr->sample_total, sizeof(uint16_t));
        cap_import_channel(abuf, ch, acap->samples);
        cap_set_analog_minmax(acap);
        /* Create a digital version of the analog capture */
        adc_acap_ttl(acap);
        TAILQ_INSERT_TAIL(bun->caps, cap, entry);
    }

    /* Unmap the capture file. */
    munmap(abuf, abuf_len);

    *new_bundle = bun;
}

/* Samples are stored as a float in the capture file; convert them
 * back to uint16_t; the goal here is to store what the hardware would
 * have spit out.
 */
static void cap_import_channel(void *abuf, unsigned ch, uint16_t *samples_out)
{
    struct saleae_analog_header *hdr = (struct saleae_analog_header *) abuf;
    float *ch_start;

    assert(NULL != abuf);
    assert(NULL != samples_out);
    assert(ch < 16);

    ch_start = (float *) (abuf + sizeof(struct saleae_analog_header) +
        (ch * (sizeof(float) * hdr->sample_total)));
    for (uint64_t i = 0; i < hdr->sample_total; i++) {
        samples_out[i] = (uint16_t) ch_start[i];
     }
}

#if 0

int saleae_import_digital(const char *cap_file, size_t sample_width, float freq, struct cap_digital **new_dcap)
{
    void *dbuf;
    size_t dbuf_len;
    struct cap_digital *dcap;
    int rc;

    rc = mmap_file(cap_file, &dbuf, &dbuf_len);
    if (rc == -ENOENT) {
        printf("Unable to open digital capture file '%s'!\n", cap_file);
        return -1;
    }

    dcap = calloc(1, sizeof(struct cap_digital));
    dcap->samples = calloc(dbuf_len, sizeof(uint32_t));
    dcap->nsamples = dbuf_len / sample_width;
    dcap->period = 1.0f/freq;

    memcpy(dcap->samples, dbuf, dbuf_len);
    munmap(dbuf, dbuf_len);
    *new_dcap = dcap;

    return 0;
}
#endif

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
