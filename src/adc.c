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

/* Static prototypes */
static int adc_convert(struct analog_cap *acap, struct digital_cap **dcap, uint16_t v_lo, uint16_t v_hi);

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

uint16_t adc_volt_to_raw(float voltage, struct adc_cal *cal)
{
    const float adc_max = 4095.0;
    float vmax, vmin;
    uint16_t adc_raw;

    /* If calibration struct provided, override default voltage swings. */
    vmax = (cal) ? cal->vmax : VMAX_DEFAULT;
    vmin = (cal) ? cal->vmin : VMIN_DEFAULT;

    adc_raw = (adc_max * (voltage - vmin)) / (vmax - vmin);
    return adc_raw;
}

int adc_ttl_convert(struct analog_cap *acap, struct digital_cap **dcap)
{
    const uint16_t ttl_low = adc_volt_to_raw(0.8f, acap->cal);
    const uint16_t ttl_high = adc_volt_to_raw(2.0f, acap->cal);
    return adc_convert(acap, dcap, ttl_low, ttl_high);
}

static int adc_convert(struct analog_cap *acap, struct digital_cap **udcap, uint16_t v_lo, uint16_t v_hi)
{
    uint32_t digital;
    struct digital_cap *dcap;

    /* Allocate a digital_cap structure with the same parameters
     * of the analog_cap
     */
    dcap = calloc(1, sizeof(struct digital_cap));
    dcap->nsamples = acap->nsamples;
    dcap->period = acap->period;
    dcap->samples = calloc(dcap->nsamples, sizeof(uint32_t));

    digital = 0;
    for (uint64_t idx = 0; idx < acap->nsamples; idx++) {
        for (uint16_t ch = 0; ch < acap->nchannels; ch++) {
            /* Digital samples only change when crossing the voltage
             * thresholds.
             */
            uint16_t ch_sample = acap->samples[ch][idx];
            if (ch_sample <= v_lo) {
                digital &= ~(1 << ch);
            } else if (ch_sample >= v_hi) {
                digital |= (1 << ch);
            }
        }
        dcap->samples[idx] = digital;
    }

    *udcap = dcap;
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
        raw_ch[i] = (uint16_t) samples[i];
    }
    *ch_buf = raw_ch;

    return 0;
}
