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

// Implement as list
struct cap_analog {
    uint64_t nsamples;
    uint8_t physical_ch;
    float period;
    struct adc_cal *cal;
    uint16_t sample_min;
    uint16_t sample_max;
    uint16_t *samples;
    struct refcnt rcnt;
};


/* Struct: cap_digital
 *
 * Container for an unprocessed digital capture.
 *
 * Fields:
 *  nsamples - Number of samples in capture
 *  period - time between each subsequent sample
 *  samples - An array of captures, bits map directly to channels.
 */
// TODO - 2016/12/15 - jbradach - add support for mapping physical channels
// TODO - to logical ones, same as for analog.  Maybe "original channel"
// TODO - would be simple enough.
struct cap_digital {
    uint64_t nsamples;
    float period;
    uint32_t *samples;
    struct refcnt rcnt;
};

// TODO - Convert acaps to be a dlink-list
struct cap_bundle {
    unsigned nacaps;
    struct cap_analog **acaps;
    struct cap_digital *dcap;
    struct refcnt rcnt;
};


#include "adc.h"

/* Analog Capture Utilities */
void cap_analog_ch_copy(struct cap_analog *acap, uint8_t from, uint32_t to);
void cap_set_analog_minmax(struct cap_analog *acap);

/* Digital Capture Utilities */
void cap_digital_ch_copy(struct cap_digital *dcap, uint8_t from, uint32_t to);

/* Capture structure allocation */
struct cap_analog *cap_analog_create(struct cap_analog *acap);
struct cap_bundle *cap_bundle_create(struct cap_bundle *bun);
struct cap_digital *cap_digital_create(struct cap_digital *dcap);

void cap_analog_set_cal(struct cap_analog *acap, struct adc_cal *cal);

/* Capture structure cleanup */
void cap_analog_destroy(struct cap_analog *acap);
void cap_bundle_destroy(struct cap_bundle *bundle);
void cap_digital_destroy(struct cap_digital *dcap);

#ifdef __cplusplus
}
#endif

#endif
