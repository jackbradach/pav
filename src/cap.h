/* File: cap.h
 *
 * Headers for routines to make working with collections of unprocessed
 * samples easier.
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
#ifndef _CAP_H_
#define _CAP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "queue.h"
#include "refcnt.h"

typedef struct cap_analog cap_analog_t;
typedef struct cap_digital cap_digital_t;
typedef struct cap_bundle cap_bundle_t;
typedef struct cap cap_t;

#include "adc.h"

uint64_t cap_find_next_edge(cap_t *cap, uint64_t from);
uint64_t cap_find_prev_edge(cap_t *cap, uint64_t from);
void cap_clone_channel_to_bundle(cap_bundle_t *bun, cap_analog_t *src, unsigned nloops, unsigned skew_us);

/* Analog Capture Utilities */
cap_t *cap_create_subcap(cap_t *cap, int64_t begin, int64_t end);
int cap_subcap_nparents(cap_t *cap);
cap_t *cap_subcap_get_top(cap_t *c);
void cap_analog_ch_copy(cap_analog_t *acap, uint8_t from, uint32_t to);
void cap_analog_set_minmax(cap_analog_t *acap);
void cap_analog_adc(cap_analog_t *acap, uint16_t v_lo, uint16_t v_hi);
void cap_analog_adc_ttl(cap_analog_t *acap);

/* Digital Capture Utilities */
void cap_digital_ch_copy(cap_digital_t *dcap, uint8_t from, uint32_t to);

/* Capture lifecycle functions */
cap_t *cap_addref(cap_t *cap);
void cap_dropref(cap_t *cap);
unsigned cap_getref(cap_t *cap);

void cap_set_nsamples(cap_t *cap, uint64_t nsamples);
uint64_t cap_get_nsamples(cap_t *cap);
void cap_set_offset(cap_t *cap, uint64_t offset);
uint64_t cap_get_offset(cap_t *cap);
const char *cap_get_note(cap_t *cap);
void cap_set_note(cap_t *cap, const char *note);
cap_t *cap_get_parent(cap_t *cap);


void cap_set_physical_ch(cap_t *cap, uint8_t ch);
uint8_t cap_get_physical_ch(cap_t *cap);
void cap_set_period(cap_t *cap, float t);
float cap_get_period(cap_t *cap);

cap_analog_t *cap_analog_create(size_t len);
cap_analog_t *cap_get_analog(cap_t *cap);
void cap_analog_set_sample(cap_analog_t *acap, uint64_t idx, uint16_t sample);
cap_digital_t *cap_analog_get_digital(cap_t *cap);

uint16_t cap_analog_get_sample(cap_analog_t *acap, uint64_t idx);
uint16_t cap_analog_get_sample_min(cap_analog_t *acap);
uint16_t cap_analog_get_sample_max(cap_analog_t *acap);
void cap_analog_set_cal(cap_analog_t *acap, float vmin, float vmax);
adc_cal_t *cap_analog_get_cal(cap_analog_t *acap);

cap_digital_t *cap_digital_create(size_t len);
cap_digital_t *cap_get_digital(cap_t *cap);
void cap_digital_set_sample(cap_digital_t *dcap, uint64_t idx, uint32_t sample);
uint32_t cap_digital_get_sample(cap_digital_t *dcap, uint64_t idx);


/* Bundle lifecycle functions */
cap_bundle_t *cap_bundle_create(void);
cap_bundle_t *cap_bundle_addref(cap_bundle_t *cap);
void cap_bundle_dropref(cap_bundle_t *cap);
unsigned cap_bundle_getref(cap_bundle_t *cap);

void cap_bundle_add(cap_bundle_t *bundle, cap_t *cap);
void cap_bundle_remove(cap_bundle_t *bundle, cap_t *cap);
cap_t *cap_bundle_get(cap_bundle_t *bundle, unsigned idx);
unsigned cap_bundle_len(cap_bundle_t *bundle);

cap_t *cap_bundle_first(cap_bundle_t *bundle);
cap_t *cap_next(cap_t *cap);
cap_t *cap_bundle_last(cap_bundle_t *bundle);

#ifdef __cplusplus
}
#endif

#endif
