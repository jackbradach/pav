#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "adc.h"

#if 0
printf("Analog buffer parser\n");
printf("Total samples: %lu\n", hdr->sample_total);
printf("Channel count: %u\n", hdr->channel_count);
printf("Sample period: %e\n", hdr->sample_period);
#endif

const float VMAX_DEFAULT = 10.0f;
const float VMIN_DEFAULT = -10.0f;

struct __attribute__((__packed__)) analog_header {
    uint64_t sample_total;
    uint32_t channel_count;
    double sample_period;
};

/* From Saleae's website */
float adc_apply_scale(uint16_t sample, struct adc_cal *cal)
{
    const float adc_max = 4095.0;
    float vmax, vmin;
    float scaled;

    /* If calibration struct provided, override default voltage swings. */
    vmax = (cal) ? cal->vmax : VMAX_DEFAULT;
    vmin = (cal) ? cal->vmin : VMIN_DEFAULT;

    scaled = (sample / adc_max) * (vmax - vmin) + vmin;
    return scaled;
}

uint16_t adc_volt_to_raw(float voltage)
{

}

void adc_print_saleae_hdr(void *abuf)
{
    struct analog_header *hdr = abuf;
    printf("Analog buffer parser\n");
    printf("Total samples: %lu\n", hdr->sample_total);
    printf("Channel count: %u\n", hdr->channel_count);
    printf("Sample period: %e\n", hdr->sample_period);
}

int adc_ch_samples(void *abuf, uint8_t ch, uint16_t **ch_buf)
{
    struct analog_header *hdr;
    float *samples;
    uint16_t *raw_ch;

    if ((NULL == abuf) || (NULL == ch_buf)) {
        return -EINVAL;
    }

    hdr = (struct analog_header *) abuf;
    if (ch >= hdr->channel_count) {
        printf("Tried to extract a non-existant channel! (%d, max %d)\n",
            ch, hdr->channel_count - 1);
        return -EINVAL;
    }

    samples = (float *) (abuf + sizeof(struct analog_header) +
                (ch * (sizeof(float) * hdr->sample_total)));
    raw_ch = (uint16_t *) calloc(hdr->sample_total, sizeof(uint16_t));

    for (uint64_t i = 0; i < hdr->sample_total; i++) {
        printf("%f \n", samples[i]);
        raw_ch[i] = (uint16_t) samples[i];
    }
    *ch_buf = raw_ch;

    return 0;
}
