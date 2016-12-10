#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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
static int mmap_file(const char *filename, size_t *length, void **buf);
static void saleae_print_analog_header(struct saleae_analog_header *hdr);


static void saleae_print_analog_header(struct saleae_analog_header *hdr)
{
    printf("Saleae Analog Capture File\n");
    printf("--------------------------\n");
    printf("Total samples: %lu\n", hdr->sample_total);
    printf("Channel count: %u\n", hdr->channel_count);
    printf("Sample period: %.02e\n", hdr->sample_period);
}

int saleae_import_analog(const char *cap_file, const char *cal_file, struct analog_cap **new_acap)
{
    void *abuf;
    size_t abuf_len;
    struct analog_cap *acap;
    struct saleae_analog_header *hdr;
    struct adc_cal *cal = NULL;
    int rc;

    rc = mmap_file(cap_file, &abuf_len, &abuf);
    if (rc == -ENOENT) {
        printf("Unable to open analog capture file '%s'!\n", cap_file);
        return -1;
    }

    hdr = (struct saleae_analog_header *) abuf;
    saleae_print_analog_header(hdr);

    /* Allocate Analog Capture structure */
    acap = calloc(1, sizeof(struct analog_cap));
    acap->nsamples = hdr->sample_total;
    acap->nchannels = hdr->channel_count;
    acap->period = hdr->sample_period;
    acap->cal = cal;
    acap->samples = calloc(acap->nchannels, sizeof(uint16_t *));
    for (uint16_t ch = 0; ch < acap->nchannels; ch++) {
        acap->samples[ch] = calloc(acap->nsamples, sizeof(uint16_t));
    }

    /* Samples are stored as a float in the capture file; convert them
     * back to uint16_t; the goal here is to store what the hardware would
     * have spit out.
     */
     for (uint16_t ch = 0; ch < acap->nchannels; ch++) {
         float *ch_start = (float *) (abuf + sizeof(struct saleae_analog_header) +
                (ch * (sizeof(float) * hdr->sample_total)));
         for (uint64_t i = 0; i < hdr->sample_total; i++) {
             acap->samples[ch][i] = (uint16_t) ch_start[i];
         }
     }

     /* The original file isn't needed anymore. */
     munmap(abuf, abuf_len);
     *new_acap = acap;

     return 0;
}

void saleae_import_digital(const char *filename, struct analog_cap **cap)
{

}

static int mmap_file(const char *filename, size_t *length, void **buf)
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

    *length = st.st_size;
    *buf = addr;

    return 0;
}
