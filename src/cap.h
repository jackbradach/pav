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

/* Struct: cap_analog
 *
 * Container for an unprocessed analog capture.
 *
 * Fields:
 *  nsamples - Number of samples in capture
 *  nchannels - Number of channels contained
 *  period - time between each subsequent sample
 *  cal - optional calibration structure (applied after processing)
 *  samples - pointers to an array of captures, one per channel.
 */
// TODO - 2016/12/15 - jbradach - add support for mapping physical channels
// TODO - to logical ones, so we can capture on eg channels 0, 7, 9, 11 and
// TODO - have it store them as logical channels [3:0].
#if 0
struct cap_analog {
    uint64_t nsamples;
    uint32_t nchannels;
    float period;
    struct adc_cal *cal;
    uint16_t **samples;
};
#endif

enum cap_types {
    CAP_TYPE_INVALID = 0,
    CAP_TYPE_ANALOG,
    CAP_TYPE_DIGITAL
};

struct cap {
    TAILQ_ENTRY(cap) entry;
    char *note[64];
    uint64_t nsamples;
    uint8_t physical_ch;
    float period;
    enum cap_types type;
    struct refcnt rcnt;
};
TAILQ_HEAD(cap_list, cap);
typedef struct cap cap_t;


struct cap_analog {
    cap_t super;

    /* Analog-specific fields */
    struct adc_cal *cal;
    uint16_t sample_min;
    uint16_t sample_max;
    uint16_t *samples;
    struct cap_digital *dcap;
};

/* Struct: cap_digital
 *
 * Container for a processed digital capture.
 *
 * Fields:
 *  nsamples - Number of samples in capture
 *  period - time between each subsequent sample
 *  samples - An array of captures, bits map directly to channels.
 */
// TODO - 2016/12/17 - jbradach - only using one-bit of sample, should
// TODO - maybe pack them in and have a 'getsample' function?
struct cap_digital {
    cap_t super;

    /* Digital-specific fields */
    uint8_t *samples;
};

// TODO - Convert caps to be a dlink-list of capture_base_t
struct cap_bundle {
    struct refcnt rcnt;
    struct cap_list *caps;
    void (*add)(struct cap_bundle *bundle, cap_t *cap);
    void (*remove)(struct cap_bundle *bundle, cap_t *cap);
    cap_t *(*get)(struct cap_bundle *bundle, unsigned idx);
    unsigned (*len)(struct cap_bundle *bundle);
};


#include "adc.h"

/* Analog Capture Utilities */
void cap_analog_ch_copy(struct cap_analog *acap, uint8_t from, uint32_t to);
void cap_set_analog_minmax(struct cap_analog *acap);

/* Digital Capture Utilities */
void cap_digital_ch_copy(struct cap_digital *dcap, uint8_t from, uint32_t to);

/* Capture lifecycle functions */
struct cap_analog *cap_analog_create(void);
struct cap_digital *cap_digital_create(void);
cap_t *cap_addref(cap_t *cap);
void cap_dropref(cap_t *cap);
unsigned cap_getref(cap_t *cap);

/* Bundle lifecycle functions */
struct cap_bundle *cap_bundle_create(void);
struct cap_bundle *cap_bundle_addref(struct cap_bundle *cap);
void cap_bundle_dropref(struct cap_bundle *cap);
unsigned cap_bundle_getref(struct cap_bundle *cap);

void cap_bundle_add(struct cap_bundle *bundle, cap_t *cap);
void cap_bundle_remove(struct cap_bundle *bundle, cap_t *cap);
cap_t *cap_bundle_get(struct cap_bundle *bundle, unsigned idx);
unsigned cap_bundle_len(struct cap_bundle *bundle);

void cap_analog_set_cal(struct cap_analog *acap, struct adc_cal *cal);






#ifdef __cplusplus
}
#endif

#endif
