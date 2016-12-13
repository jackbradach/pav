#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "adc.h"
#include "cap.h"

#if 0
printf("Analog buffer parser\n");
printf("Total samples: %lu\n", hdr->sample_total);
printf("Channel count: %u\n", hdr->channel_count);
printf("Sample period: %e\n", hdr->sample_period);
#endif

const float VMAX_DEFAULT = 10.0f;
const float VMIN_DEFAULT = -10.0f;


/* Static prototypes */
static int adc_convert(struct cap_analog *acap, struct cap_digital **dcap, uint16_t v_lo, uint16_t v_hi);

/* From Saleae's website */
float adc_sample_to_voltage(uint16_t sample, struct adc_cal *cal)
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

uint16_t adc_voltage_to_sample(float voltage, struct adc_cal *cal)
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

int adc_ttl_convert(struct cap_analog *acap, struct cap_digital **dcap)
{
    const uint16_t ttl_low = adc_voltage_to_sample(0.8f, acap->cal);
    const uint16_t ttl_high = adc_voltage_to_sample(2.0f, acap->cal);
    return adc_convert(acap, dcap, ttl_low, ttl_high);
}

static int adc_convert(struct cap_analog *acap, struct cap_digital **udcap, uint16_t v_lo, uint16_t v_hi)
{
    uint32_t digital;
    struct cap_digital *dcap;

    /* Allocate a cap_digital structure with the same parameters
     * of the cap_analog
     */
    dcap = calloc(1, sizeof(struct cap_digital));
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
