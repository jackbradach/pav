/* File: adc.h
 *
 * Analog to Digital conversion headers
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
#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct adc_cal adc_cal_t;

#include "cap.h"

adc_cal_t *adc_cal_create(float vmin, float vmax);
float adc_sample_to_voltage(uint16_t sample, adc_cal_t *cal);
uint16_t adc_voltage_to_sample(float voltage, adc_cal_t *cal);
void adc_acap_ttl(cap_analog_t *acap);
void adc_acap(cap_analog_t *acap, uint16_t v_lo, uint16_t v_hi);

double adc_cal_get_vmin(adc_cal_t *cal);
double adc_cal_get_vmax(adc_cal_t *cal);

#ifdef __cplusplus
}
#endif

#endif
