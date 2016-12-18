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

static void cap_bundle_free(const struct refcnt *ref);
static void cap_analog_free(const struct refcnt *ref);
static void cap_digital_free(const struct refcnt *ref);

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
void cap_set_analog_minmax(struct cap_analog *acap)
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

/* Function: cap_analog_create
 *
 * Allocates and initializes a new analog capture structure
 * or increments a reference to an existing one.
 *
 * Parameters:
 *  acap - if not null, will have its refcnt incremented
 *
 * Returns:
 *  pointer to new or existing cap_analog.
 *
 * See Also:
 *  <cap_analog_destroy>
 */
struct cap_analog *cap_analog_create(struct cap_analog *acap)
{
    struct cap_analog *cap;

    if (acap) {
        refcnt_inc(&acap->rcnt);
        cap = acap;
    } else {
        cap = calloc(1, sizeof(struct cap_analog));
        cap->rcnt = (struct refcnt) { cap_analog_free, 1 };
    }

    return cap;
}

/* Function: cap_analog_destroy
 *
 * Mark an analog capture structure as no longer used; if no otherwise
 * references are held to the structure, it'll be cleaned up and the
 * memory freed.
 *
 * Parameters:
 *  acap - handle to an analog capture structure
 */
void cap_analog_destroy(struct cap_analog *acap)
{
    assert(NULL != acap);
    refcnt_dec(&acap->rcnt);
}

/* Function: cap_analog_free
 *
 * Cleanup callback for an analog capture structure; this gets
 * called automatically by cap_analog_destroy when the structure
 * has no more references.
 *
 * Parameters:
 *  ref - handle to a reference counter embedded in a capture bundle
 */
static void cap_analog_free(const struct refcnt *ref)
{
    struct cap_analog *acap =
        container_of(ref, struct cap_analog, rcnt);

    assert(NULL != acap);

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

/* Function: cap_bundle_create
 *
 * Allocates and initializes a new capture bundle
 * or increments a reference to an existing one.
 *
 * Parameters:
 *  bun - if not null, will have its refcnt incremented
 *
 * Returns:
 *  pointer to new or existing cap_analog.
 *
 * See Also:
 *  <cap_bundle_destroy>
 */
struct cap_bundle *cap_bundle_create(struct cap_bundle *bun)
{
    struct cap_bundle *b;
    if (bun) {
        refcnt_inc(&bun->rcnt);
        b = bun;
    } else {
        b = calloc(1, sizeof(struct cap_bundle));
        b->rcnt = (struct refcnt) { cap_bundle_free, 1 };
    }
    return b;
}

/* Function: cap_bundle_destroy
 *
 * Mark a capture bundle as no longer used; if no other references are
 * held to the structure, it'll be cleaned up and the memory freed.
 *
 * Parameters:
 *  bundle - valid handle to an capture bundle structure
 *
 * See Also:
 *  <cap_bundle_create>
 */
void cap_bundle_destroy(struct cap_bundle *bundle)
{
    refcnt_dec(&bundle->rcnt);
}

/* Function: cap_bundle_free
 *
 * Cleanup callback for a capture bundle; this gets called automatically
 * by cap_bundle_destroy when the structure has no more references.
 *
 * Parameters:
 *  ref - handle to a reference counter embedded in a capture bundle
 */
static void cap_bundle_free(const struct refcnt *ref)
{
    struct cap_bundle *bundle =
        container_of(ref, struct cap_bundle, rcnt);

    assert(NULL != bundle);

    /* Drop reference count on all child structures. */
    if (bundle->acaps) {
        for (unsigned i = 0; i < bundle->nacaps; i++) {
            if (bundle->acaps[i]) {
                cap_analog_destroy(bundle->acaps[i]);
                bundle->acaps[i] = NULL;
            }
        }
        free(bundle->acaps);
        bundle->acaps = NULL;
    }

    free (bundle);
}

/* Function: cap_digital_create
 *
 * Allocates and initializes a new digital capture structure
 * or increments a reference to an existing one.
 *
 * Parameters:
 *  dcap - if not null, will have its refcnt incremented
 *
 * Returns:
 *  pointer to new or existing cap_digital.
 *
 * See Also:
 *  <cap_digital_destroy>
 */
struct cap_digital *cap_digital_create(struct cap_digital *dcap)
{
    struct cap_digital *cap;

    if (dcap) {
        refcnt_inc(&dcap->rcnt);
        cap = dcap;
    } else {
        cap = calloc(1, sizeof(struct cap_digital));
        cap->rcnt = (struct refcnt) { cap_digital_free, 1 };
    }

    return cap;
}

/* Function: cap_digital_destroy
 *
 * Mark an digital capture structure as no longer used; if no otherwise
 * references are held to the structure, it'll be cleaned up and the
 * memory freed.
 *
 * Parameters:
 *  dcap - handle to an digital capture structure
 */
void cap_digital_destroy(struct cap_digital *dcap)
{
    assert(NULL != dcap);
    refcnt_dec(&dcap->rcnt);
}

/* Function: cap_digital_free
 *
 * Cleanup callback for an digital capture structure; this gets
 * called automatically by cap_digital_destroy when the structure
 * has no more references.
 *
 * Parameters:
 *  ref - handle to a reference counter embedded in a digital capture
 */
static void cap_digital_free(const struct refcnt *ref)
{
    struct cap_digital *dcap =
        container_of(ref, struct cap_digital, rcnt);

    assert(NULL != dcap);

    if (dcap->samples)
        free(dcap->samples);

    free(dcap);
}

void cap_analog_set_cal(struct cap_analog *acap, struct adc_cal *cal)
{
    acap->cal = cal;
}
