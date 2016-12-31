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

typedef struct cap cap_t;
typedef struct cap_bundle cap_bundle_t;

#include "adc.h"

uint64_t cap_next_edge(cap_t *c, uint64_t from);
uint64_t cap_prev_edge(cap_t *c, uint64_t from);
void cap_clone_to_bundle(cap_bundle_t *bun, cap_t *src, unsigned nloops, unsigned skew_us);
cap_t *cap_create_subcap(cap_t *c, int64_t begin, int64_t end);
cap_t *cap_get_parent(cap_t *cap);
int cap_get_nparents(cap_t *c);
cap_t *cap_get_top(cap_t *c);

void cap_update_analog_minmax(cap_t *c);
void cap_analog_adc(cap_t *c, uint16_t v_lo, uint16_t v_hi);
void cap_analog_adc_ttl(cap_t *c);

/* Capture lifecycle functions */
cap_t *cap_create(size_t len);
cap_t *cap_addref(cap_t *c);
void cap_dropref(cap_t *c);
unsigned cap_nref(cap_t *c);

const char *cap_get_note(cap_t *c);
void cap_set_note(cap_t *cap, const char *note);

uint64_t cap_get_nsamples(cap_t *c);

uint64_t cap_get_offset(cap_t *c);
void cap_set_offset(cap_t *c, uint64_t n);

void cap_set_period(cap_t *cap, float t);
float cap_get_period(cap_t *cap);

uint8_t cap_get_physical_ch(cap_t *c);
void cap_set_physical_ch(cap_t *c, uint8_t ch);

uint16_t cap_get_analog(cap_t *c, uint64_t idx);
void cap_set_analog(cap_t *c, uint64_t idx, uint16_t sample);
uint16_t cap_get_analog_min(cap_t *c);
float cap_get_analog_vmin(struct cap *c);
uint16_t cap_get_analog_max(cap_t *c);
float cap_get_analog_vmax(struct cap *c);
float cap_get_analog_voltage(struct cap *c, uint64_t idx);
void cap_set_analog_cal(cap_t *c, float vmin, float vmax);
adc_cal_t *cap_get_analog_cal(cap_t *c);

uint8_t cap_get_digital(cap_t *c, uint64_t idx);
void cap_set_digital(cap_t *c, uint64_t idx, uint8_t sample);

/* Bundle lifecycle functions */
cap_bundle_t *cap_bundle_create(void);
cap_bundle_t *cap_bundle_addref(cap_bundle_t *b);
void cap_bundle_dropref(cap_bundle_t *b);
unsigned cap_bundle_nref(cap_bundle_t *b);

void cap_bundle_add(cap_bundle_t *b, cap_t *c);
void cap_bundle_remove(cap_bundle_t *b, cap_t *c);
cap_t *cap_bundle_get(cap_bundle_t *b, unsigned idx);
unsigned cap_bundle_len(cap_bundle_t *b);

cap_t *cap_bundle_first(cap_bundle_t *b);
cap_t *cap_next(cap_t *c);
cap_t *cap_bundle_last(cap_bundle_t *b);

#ifdef __cplusplus
}
#endif

#endif
