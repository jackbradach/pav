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
