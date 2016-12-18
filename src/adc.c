/* File: adc.c
 *
 * Analog to Digital conversion functions
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
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "adc.h"
#include "cap.h"


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

// FIXME: update these to use the new analog capture and bundle formats.
#if 0
int adc_ttl_convert(struct cap_analog *acap, struct cap_digital **dcap)
{
    const uint16_t ttl_low = adc_voltage_to_sample(0.8f, acap->cal);
    const uint16_t ttl_high = adc_voltage_to_sample(2.0f, acap->cal);
    return adc_convert(acap, dcap, ttl_low, ttl_high);
}

// TODO - rewrite this so you give it a bundle and it updates the digital channels
// TODO - with the ADC from the analog.  Need to have a way to prevent an
// TODO - unrelated analog channel from being included!
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
#endif
