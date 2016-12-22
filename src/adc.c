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

/* Struct: adc_cal
 *
 * Calibration values for analog samples, these are used to convert
 * the raw sample values to voltages according to the calibration
 * from the logic analyzer.  Scale defaults to +/- 10.0V if not
 * provided.
 */
struct adc_cal {
    double vmin;
    double vmax;
};

struct adc_cal *adc_cal_create(float vmin, float vmax)
{
    struct adc_cal *cal = calloc(1, sizeof(struct adc_cal));
    cal->vmin = vmin;
    cal->vmax = vmax;
    return cal;
}

double adc_cal_get_vmin(adc_cal_t *cal)
{
    return cal->vmin;
}

double adc_cal_get_vmax(adc_cal_t *cal)
{
    return cal->vmax;
}


/* Static prototypes */
//static int adc_convert(struct cap_analog *acap, struct cap_digital **dcap, uint16_t v_lo, uint16_t v_hi);

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

void adc_acap_ttl(struct cap_analog *acap)
{
    const uint16_t ttl_low = adc_voltage_to_sample(0.8f, cap_analog_get_cal(acap));
    const uint16_t ttl_high = adc_voltage_to_sample(2.0f, cap_analog_get_cal(acap));
    adc_acap(acap, ttl_low, ttl_high);
}

void adc_acap(struct cap_analog *acap, uint16_t v_lo, uint16_t v_hi)
{
    unsigned nsamples;
    struct cap_digital *dcap;

    /* Shred any existing digital capture */
    // TODO - 2016/12/17 - jbradach - this probably ought to allow merging
    // TODO - with existing digital samples.  Also, any 'updating' is really
    // TODO - more abandoning the old dcap (it'll be freed if no one holds a
    // TODO - handle) and creating a new one.  This'll make copying them around
    // TODO - less of a headache.
    nsamples = cap_get_nsamples((cap_t *) acap);
    dcap = cap_digital_create(nsamples);

    for (uint64_t i = 0; i < nsamples; i++) {
        static uint8_t digital = 0;
        /* Digital samples only change when crossing the voltage
         * thresholds.
         */
        uint16_t ch_sample = cap_analog_get_sample(acap, i);
        if (ch_sample <= v_lo) {
            digital = 0;
        } else if (ch_sample >= v_hi) {
            digital = 1;
        }
        cap_digital_set_sample(dcap, i, digital);
    }

    cap_analog_set_dcap(acap, dcap);
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
