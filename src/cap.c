/* File: cap.c
 *
 * Routines for working with captures (collections of unprocessed samples)
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
#include <assert.h>
#include <stdint.h>

#include "cap.h"

void cap_analog_free(struct cap_analog *acap)
{
    if (NULL == acap)
        return;

    for(int i = 0; i < acap->nchannels; i++) {
        free(acap->samples[i]);
    }

    if (acap->samples)
        free(acap->samples);
    free(acap);
}

void cap_digital_free(struct cap_digital *dcap)
{
    if (dcap->samples)
        free(dcap->samples);
    free(dcap);
}

/* Function: cap_analog_ch_copy
 *
 * Replicates the data from a source channel to targets, specified by mask.
 * Each channel consumes uint16_t * nsamples of memory.
 * TODO - 2016/12/12 - jbradach - make this more efficient by setting up aliases
 * TODO - that map logical channels to physical ones.
 *
 * Parameters:
 *      from - channel ID for source data_valid
 *      to - 32-bit mask specifying destination channels.  This mask must not
 *          include the source channel!
 */
void cap_analog_ch_copy(struct cap_analog *acap, uint8_t from, uint32_t to)
{
    assert(from < 32);
    assert((1 << from) & to);

    // TODO - finish me!
    abort();
    for (uint64_t i = 0; i < acap->nsamples; i++) {
    }
}

/* Function: cap_digital_ch_copy
 *
 * Replicates the data from a source channel to targets, specified by mask.
 * This is "free" memory-wise, as internally digital captures are saved
 * as a 32-bit mask representing the channels.
 *
 * Parameters:
 *      from - channel ID for source data_valid
 *      to - 32-bit mask specifying destination channels.  This mask must not
 *          include the source channel!
 */
void cap_digital_ch_copy(struct cap_digital *dcap, uint8_t from, uint32_t to)
{
    assert(from < 32);
    assert((1 << from) & to);

    for (uint64_t i = 0; i < dcap->nsamples; i++) {
        uint32_t tmp = dcap->samples[i] &= ~to;

        if (dcap->samples[i] & (1 << from)) {
            tmp |= dcap->samples[i];
        }
        dcap->samples[i] = tmp;
    }
}

/* Populates the min/max fields of an acap */
void cap_set_analog_minmax(struct cap_analog_new *acap)
{
    uint16_t min = 4095;
    uint16_t max = 0;

    assert(NULL != acap);

    for (uint64_t i = 0; i < acap->nsamples; i++) {
        uint16_t sample = acap->samples[i];
        if (sample < min) {
            min = sample;
        } else if (sample > max) {
            max = sample;
        }
    }

    acap->sample_min = min;
    acap->sample_max = max;
}

void cap_analog_new_free(struct cap_analog_new *acap)
{
    if (NULL == acap)
        return;

    if (acap->cal) {
        free(acap->cal);
        acap->cal = NULL;
    }

    if (acap->samples) {
        free(acap->samples);
        acap->samples = NULL;
    }

    free(acap);
}

void cap_bundle_free(struct cap_bundle *bundle)
{
    if (NULL == bundle)
        return;

    /* Call analog capture destructor for all in the bundle. */
    if (bundle->acaps) {
        for (unsigned i = 0; i < bundle->nacaps; i++) {
            if (bundle->acaps[i]) {
                cap_analog_new_free(bundle->acaps[i]);
                bundle->acaps[i] = NULL;
            }
        }
        free(bundle->acaps);
        bundle->acaps = NULL;
    }

    free (bundle);
}
