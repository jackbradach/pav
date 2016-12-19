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

#include "cap.h"

float adc_sample_to_voltage(uint16_t sample, struct adc_cal *cal);
uint16_t adc_voltage_to_sample(float voltage, struct adc_cal *cal);
void adc_acap_ttl(struct cap_analog *acap);
void adc_acap(struct cap_analog *acap, uint16_t v_lo, uint16_t v_hi);

#ifdef __cplusplus
}
#endif

#endif
