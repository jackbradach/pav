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
#include "queue.h"

/* Static prototypes - these get automatically called by
 * the reference counter.
 */
static void cap_free(const struct refcnt *ref);
static void cap_analog_free(struct cap_analog *acap);
static void cap_digital_free(struct cap_digital *dcap);
static void cap_bundle_free(const struct refcnt *ref);

#if 0

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
#endif

/* Populates the min/max fields of an acap */
void cap_set_analog_minmax(struct cap_analog *acap)
{
    uint16_t min = 4095;
    uint16_t max = 0;
    unsigned nsamples;

    assert(NULL != acap);

    nsamples = acap->super.nsamples;
    for (uint64_t i = 0; i < nsamples; i++) {
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
struct cap_analog *cap_analog_create(void)
{
    struct cap_analog *cap;
    cap = calloc(1, sizeof(struct cap_analog));
    cap->super.rcnt = (struct refcnt) { cap_free, 1 };
    cap->super.type = CAP_TYPE_ANALOG;
    return cap;
}


/* Function: cap_add_ref
 *
 * Adds a reference to a capture structure and returns a pointer
 * for the caller to use.  Caller should use this instead of the
 * original pointer in case a new structure needed to be created.
 *
 * Parameters:
 *  cap - handle to a capture structure
 *
 * Returns:
 *  pointer to capture structure
 *
 * See Also:
 *  <cap_dropref>
 */
cap_base_t *cap_addref(cap_base_t *cap)
{
    assert(NULL != cap);
    refcnt_inc(&cap->rcnt);
    return cap;
}

/* Function: cap_dropref
 *
 * Removes a reference to a capture structure, freeing the capture
 * structure if  and returns a pointer
 * for the caller to use.  Caller should use this instead of the
 * original pointer in case a new structure needed to be created.
 *
 * Parameters:
 *  cap - handle to a capture structure
 *
 * Returns:
 *  pointer to capture structure
 *
 * See Also:
 *  <cap_dropref>
 */
void cap_dropref(cap_base_t *cap)
{
    assert(NULL != cap);
    refcnt_dec(&cap->rcnt);
}

/* Function: cap_getref
 *
 * Returns the number of active references to a capture
 *
 * Parameters:
 *  cap - handle to a capture structure
 *
 * Returns:
 *  Number of active references
 *
 * See Also:
 *  <cap_addref>, <cap_dropref>
 */
unsigned cap_getref(cap_base_t *cap)
{
    assert(NULL != cap);
    return cap->rcnt.count;
}

/* Function: cap_free
 *
 * Cleanup callback for capture structures.  It calls the
 * correct analog/digital cleanup based on the capture type.
 * This will be called automatically by cap_drop_ref when
 * the structure has no more references.
 *
 * Parameters:
 *  ref - handle to a reference counter embedded in a capture bundle
 */
static void cap_free(const struct refcnt *ref)
{
    cap_base_t *cap =
        container_of(ref, cap_base_t, rcnt);

    assert(NULL != cap);

    /* Clean up child structure */
    switch (cap->type) {
        case CAP_TYPE_ANALOG:
            cap_analog_free((struct cap_analog *) cap);
            break;
        case CAP_TYPE_DIGITAL:
            cap_digital_free((struct cap_digital *) cap);
            break;
        default:
            /* Abort on invalid type! */
            abort();
            break;
    }

    if (cap->note) {
        free(cap->note);
    }

    free(cap);
}

/* Function: cap_analog_free
 *
 * Cleanup callback for an analog capture structure; this gets
 * called automatically by cap_drop_ref when the structure
 * has no more references.
 *
 * Parameters:
 *  acap - handle to a cap_analog struct
 */
static void cap_analog_free(struct cap_analog *acap)
{
    assert(NULL != acap);

    if (acap->cal) {
        free(acap->cal);
        acap->cal = NULL;
    }

    if (acap->samples) {
        free(acap->samples);
        acap->samples = NULL;
    }
}

/* Function: cap_digital_create
 *
 * Allocates and initializes a new digital capture structure.
 *
 * Returns:
 *  pointer to new cap_digital.
 *
 * See Also:
 *  <cap_digital_destroy>
 */
struct cap_digital *cap_digital_create(void)
{
    struct cap_digital *cap;
    cap = calloc(1, sizeof(struct cap_digital));
    cap->super.rcnt = (struct refcnt) { cap_free, 1 };
    cap->super.type = CAP_TYPE_DIGITAL;
    return cap;
}

/* Function: cap_digital_free
 *
 * Cleanup callback for an digital capture structure; this gets
 * called automatically by cap_drop_ref when the structure
 * has no more references.
 *
 * Parameters:
 *  dcap - handle to a cap_digital struct
 */
static void cap_digital_free(struct cap_digital *dcap)
{
    assert(NULL != dcap);

    if (dcap->samples)
        free(dcap->samples);
}

/* Function: cap_bundle_create
 *
 * Allocates and initializes a new capture bundle.
 *
 * Returns:
 *  pointer to new capture bundle.
 *
 * See Also:
 *  <cap_bundle_destroy>
 */
struct cap_bundle *cap_bundle_create(void)
{
    struct cap_bundle *b;
    b = calloc(1, sizeof(struct cap_bundle));
    b->rcnt = (struct refcnt) { cap_bundle_free, 1 };

    /* Set up an empty capture list */
    b->caps = calloc(1, sizeof(struct cap_list));
    TAILQ_INIT(b->caps);

    /* Map function pointers */
#if 0
    b->add = &cap_bundle_add;
    b->remove = &cap_bundle_remove;
    b->get = &cap_bundle_get;
    b->len = &cap_bundle_len;
#endif
    return b;
}

/* Function: cap_bundle_addref
 *
 * Adds a reference to a capture bundle and returns a pointer
 * for the caller to use.  Caller should use this instead of the
 * original pointer in case a new structure needed to be created.
 *
 * Parameters:
 *  b - handle to a capture bundle structure
 *
 * Returns:
 *  pointer to capture bundle structure
 *
 * See Also:
 *  <cap_dropref>
 */
struct cap_bundle *cap_bundle_addref(struct cap_bundle *b)
{
    assert(NULL != b);
    refcnt_inc(&b->rcnt);
    return b;
}

/* Function: cap_bundle_dropref
 *
 * Removes a reference to a capture bundle, freeing the capture
 * structure if  and returns a pointer
 * for the caller to use.  Caller should use this instead of the
 * original pointer in case a new structure needed to be created.
 *
 * Parameters:
 *  cap - handle to a capture structure
 *
 * Returns:
 *  pointer to capture structure
 *
 * See Also:
 *  <cap_bundle_create>, <cap_bundle_addref>
 */
void cap_bundle_dropref(struct cap_bundle *b)
{
    assert(NULL != b);
    refcnt_dec(&b->rcnt);
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
    struct cap_base *cur, *tmp;
    struct cap_bundle *b =
        container_of(ref, struct cap_bundle, rcnt);

        /* Drop reference count on all child structures. */
    assert(NULL != b);
    TAILQ_FOREACH_SAFE(cur, b->caps, entry, tmp) {
        TAILQ_REMOVE(b->caps, cur, entry);
        cap_dropref(cur);
    }
    free(b->caps);
    free(b);
}



void cap_analog_set_cal(struct cap_analog *acap, struct adc_cal *cal)
{
    acap->cal = cal;
}
