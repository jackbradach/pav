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
#include <stdio.h>
#include <string.h>

#include "adc.h"
#include "cap.h"
#include "queue.h"

#define CAP_MAX_NOTE_LEN 64

enum cap_types {
    CAP_TYPE_INVALID = 0,
    CAP_TYPE_ANALOG,
    CAP_TYPE_DIGITAL
};

struct cap {
    TAILQ_ENTRY(cap) entry;
    /* Parent for a subcap (NULL for top level) */
    struct cap *parent;
    char note[64];
    uint64_t nsamples;
    uint64_t offset;
    uint8_t physical_ch;
    float period;
    enum cap_types type;
    struct refcnt rcnt;
};
TAILQ_HEAD(cap_list, cap);

struct cap_analog {
    cap_t super;
    adc_cal_t *cal;
    uint16_t sample_min;
    uint16_t sample_max;
    uint16_t *samples;
    cap_digital_t *dcap;
};

struct cap_digital {
    cap_t super;
    uint8_t *samples;
};

struct cap_bundle {
    struct refcnt rcnt;
    struct cap_list head;
    void (*add)(struct cap_bundle *bundle, cap_t *cap);
    void (*remove)(struct cap_bundle *bundle, cap_t *cap);
    cap_t *(*get)(struct cap_bundle *bundle, unsigned idx);
    unsigned (*len)(struct cap_bundle *bundle);
};

/* Static prototypes - these get automatically called by
 * the reference counter.
 */
static void cap_free(const struct refcnt *ref);
static void cap_analog_free(struct cap_analog *acap);
static void cap_digital_free(struct cap_digital *dcap);
static void cap_bundle_free(const struct refcnt *ref);

/* Populates the min/max fields of an acap */
void cap_analog_set_minmax(struct cap_analog *acap)
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

void cap_analog_adc_ttl(struct cap_analog *acap)
{
    const uint16_t ttl_low = adc_voltage_to_sample(0.8f, acap->cal);
    const uint16_t ttl_high = adc_voltage_to_sample(2.0f, acap->cal);
    cap_analog_adc(acap, ttl_low, ttl_high);
}

void cap_analog_adc(struct cap_analog *acap, uint16_t v_lo, uint16_t v_hi)
{
    unsigned nsamples;
    struct cap_digital *dcap;

    nsamples = cap_get_nsamples((cap_t *) acap);
    dcap = cap_digital_create(nsamples);

    for (uint64_t i = 0; i < nsamples; i++) {
        static uint8_t digital = 0;
        /* Digital samples only change when crossing the voltage
         * thresholds.
         */
        uint16_t ch_sample = cap_analog_get_sample(acap, i);
        if (ch_sample <= v_lo) {
            digital = 0;
        } else if (ch_sample >= v_hi) {
            digital = 1;
        }
        dcap->samples[i] = digital;
    }

    /* Shred any existing digital capture */
    if (acap->dcap) {
        cap_dropref((cap_t *) acap->dcap);
    }

    acap->dcap = dcap;
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
struct cap_analog *cap_analog_create(size_t len)
{
    struct cap_analog *cap;
    cap = calloc(1, sizeof(struct cap_analog));
    cap->super.rcnt = (struct refcnt) { cap_free, 1 };
    cap->super.type = CAP_TYPE_ANALOG;
    cap->super.nsamples = len;
    cap->samples = calloc(len, sizeof(uint16_t));
    return cap;
}


/* Function: cap_addref
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
struct cap *cap_addref(struct cap *cap)
{
    if (cap == NULL)
        return NULL;

    /* Make sure to add a ref on any digital sub-caps! */
    if (CAP_TYPE_ANALOG == cap->type) {
        struct cap_analog *acap = (struct cap_analog *) cap;
        if (acap->dcap) {
            struct cap *dcap = (struct cap*) acap->dcap;
            refcnt_inc(&dcap->rcnt);
        }
    }
    refcnt_inc(&cap->rcnt);

    return cap;
}

/* Function: cap_dropref
 *
 * Removes a reference to a capture structure, freeing the capture
 * structure when there are no more references.
 *
 * Parameters:
 *  cap - handle to a capture structure
 *
 * Returns:
 *  pointer to capture structure
 *
 * See Also:
 *  <cap_addref>
 */
void cap_dropref(struct cap *c)
{
    if (NULL == c)
        return;

    /* Make sure to drop ref on any digital sub-caps! */
    if (CAP_TYPE_ANALOG == c->type) {
        struct cap_analog *acap = (struct cap_analog *) c;
        if (acap->dcap) {
            struct cap *dcap = (struct cap*) acap->dcap;
            refcnt_dec(&dcap->rcnt);
        }
    }

    if (c->parent) {
        cap_dropref(c->parent);
        c->parent = NULL;
    }

    refcnt_dec(&c->rcnt);
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
unsigned cap_getref(cap_t *cap)
{
    if (NULL == cap)
        return 0;
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
    cap_t *cap =
        container_of(ref, cap_t, rcnt);

    /* Clean up child structure */
    switch (cap->type) {
        case CAP_TYPE_ANALOG:
            cap_analog_free((struct cap_analog *) cap);
            break;
        case CAP_TYPE_DIGITAL:
            cap_digital_free((struct cap_digital *) cap);
            break;
        default:
            /* Shouldn't ever get here. */
            break;
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
    if (acap->cal) {
        free(acap->cal);
        acap->cal = NULL;
    }

    if (acap->samples) {
        free(acap->samples);
        acap->samples = NULL;
    }
}

/* Function: cap_
 *
 * Allocates and initializes a new digital capture structure.
 *
 * Returns:
 *  pointer to new cap_digital.
 *
 * See Also:
 *  <cap_digital_destroy>
 */
struct cap_digital *cap_digital_create(size_t len)
{
    struct cap_digital *dcap;
    dcap = calloc(1, sizeof(struct cap_digital));
    dcap->super.rcnt = (struct refcnt) { cap_free, 1 };
    dcap->super.type = CAP_TYPE_DIGITAL;
    dcap->super.nsamples = len;
    dcap->samples = calloc(len, sizeof(uint32_t));
    return dcap;
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
    TAILQ_INIT(&b->head);

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
    if (NULL == b)
        return NULL;
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
    if (NULL == b)
        return;
    refcnt_dec(&b->rcnt);
}

/* Function: cap_bundle_getref
 *
 * Returns the number of active references to a capture bundle
 *
 * Parameters:
 *  b - handle to a capture bundle
 *
 * Returns:
 *  Number of active references
 *
 * See Also:
 *  <cap_bundle_addref>, <cap_bundle_dropref>
 */
unsigned cap_bundle_getref(struct cap_bundle *b)
{
    if (NULL == b)
        return 0;
    return b->rcnt.count;
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
    cap_t *cur, *tmp;
    struct cap_bundle *b =
        container_of(ref, struct cap_bundle, rcnt);

    /* Drop reference count on all child structures. */
    TAILQ_FOREACH_SAFE(cur, &b->head, entry, tmp) {
        TAILQ_REMOVE(&b->head, cur, entry);
        cap_dropref(cur);
    }
    free(b);
}

void cap_analog_set_dcap(struct cap_analog *acap, cap_digital_t *dcap)
{
    if (acap->dcap)
        cap_dropref((cap_t *) acap->dcap);
    acap->dcap = dcap;
}

void cap_analog_set_sample(struct cap_analog *acap, uint64_t idx, uint16_t sample)
{
    acap->samples[idx] = sample;
}

uint64_t cap_get_nsamples(cap_t *cap)
{
    return cap->nsamples;
}

void cap_digital_set_sample(cap_digital_t *dcap, uint64_t idx, uint32_t sample)
{
    dcap->samples[idx] = sample;
}

uint32_t cap_digital_get_sample(cap_digital_t *dcap, uint64_t idx)
{
    return dcap->samples[idx];
}



uint16_t cap_analog_get_sample(struct cap_analog *acap, uint64_t idx)
{
    return acap->samples[idx];
}

uint16_t cap_analog_get_sample_min(struct cap_analog *acap)
{
    return acap->sample_min;
}

uint16_t cap_analog_get_sample_max(struct cap_analog *acap)
{
    return acap->sample_max;
}

void cap_set_physical_ch(cap_t *cap, uint8_t ch)
{
    cap->physical_ch = ch;
}

uint8_t cap_get_physical_ch(cap_t *cap)
{
    return cap->physical_ch;
}

void cap_set_period(cap_t *cap, float t)
{
    cap->period = t;
}

float cap_get_period(cap_t *cap)
{
    return cap->period;
}

void cap_bundle_add(struct cap_bundle *b, struct cap *c)
{
    TAILQ_INSERT_TAIL(&b->head, c, entry);
}

struct cap *cap_bundle_first(struct cap_bundle *b)
{
    return TAILQ_FIRST(&b->head);
}

struct cap *cap_next(struct cap *c)
{
    return TAILQ_NEXT(c, entry);
}

struct cap *cap_bundle_last(struct cap_bundle *b)
{
    return TAILQ_LAST(&b->head, cap_list);
}

void cap_analog_set_cal(struct cap_analog *acap, float vmin, float vmax)
{
    if (acap->cal)
        free(acap->cal);
    acap->cal = adc_cal_create(vmin, vmax);
}

adc_cal_t *cap_analog_get_cal(struct cap_analog *acap)
{
    return acap->cal;
}

void cap_set_offset(struct cap *cap, uint64_t offset)
{
    cap->offset = offset;
}

uint64_t cap_get_offset(cap_t *cap)
{
    return cap->offset;
}

const char *cap_get_note(cap_t *cap)
{
    return (const char *) &cap->note;
}

void cap_set_note(cap_t *cap, const char *note)
{
    strncpy(cap->note, note, CAP_MAX_NOTE_LEN);
}

/* Function: cap_create_subcap
 *
 * Creates a subcapture, which is a structure containing a
 * slice of the samples in the parent structure.  It should be
 * treated identically to a normal capture structure and needs to
 * be dropref'd when done for garbage collection.
 */
cap_t *cap_create_subcap(struct cap *cap, int64_t begin, int64_t end)
{
    struct cap *c;
    size_t len = end - begin;

    if (begin < 0)
        begin = 0;

    if (end >= cap->nsamples)
        end = cap->nsamples - 1;

    /* Clone the analog or digital structure as appropriate. */
    if (CAP_TYPE_ANALOG == cap->type) {
        struct cap_analog *ac;
        struct cap_analog *ac_src = (struct cap_analog *) cap;
        ac = cap_analog_create(len);
        memcpy(ac->samples, ac_src->samples + begin, len * sizeof(uint16_t));

        /* Note that we don't update the min/max
         * on a subcap; it should be the same as the parent!
         */
        ac->sample_min = cap_analog_get_sample_min((cap_analog_t *) cap);
        ac->sample_max = cap_analog_get_sample_max((cap_analog_t *) cap);
        ac->cal = cap_analog_get_cal((cap_analog_t *) cap);

        /* Update the digital view for the subcapture. */
        cap_analog_adc_ttl(ac);
        c = (cap_t *) ac;
    } else {
        struct cap_digital *dc;
        struct cap_digital *dc_src = (struct cap_digital *) cap;
        dc = cap_digital_create(len);
        memcpy(dc->samples, dc_src->samples + (cap->offset + begin) * sizeof(uint8_t), len * sizeof(uint32_t));
        c = (cap_t *) dc;
    }

    /* Copy the rest of the capture structure data */
    c->parent = cap_addref(cap);
    cap_set_note(c, cap_get_note(cap));
    c->physical_ch = cap->physical_ch;
    c->period = cap->period;
    c->offset = begin + cap->offset;

    return c;
}

struct cap *cap_get_parent(struct cap *c)
{
    return c->parent;
}

struct cap *cap_subcap_get_top(struct cap *cap)
{
    struct cap *c = cap;

    while (NULL != c->parent) {
        c = c->parent;
    }
    return c;
}

int cap_subcap_nparents(struct cap *cap)
{
    struct cap *c = cap;
    int nparents = 0;

    while (NULL != c->parent) {
        nparents++;
        c = c->parent;
    }
    return nparents;
}

uint64_t cap_find_next_edge(struct cap *cap, uint64_t from)
{
    struct cap_digital *dc;
    uint64_t next;

    dc = cap_get_digital(cap);

    for (next = from + 1; next < cap->nsamples; next++) {
        if (dc->samples[from] != dc->samples[next]) {
            break;
        }
    }
    return next;
}

uint64_t cap_find_prev_edge(struct cap *cap, uint64_t from)
{
    struct cap_digital *dc;
    uint64_t prev = from;

    dc = cap_get_digital(cap);

    for (prev = from - 1; prev > 0; prev--) {
        if (dc->samples[from] != dc->samples[prev])
            break;
    }
    return prev;
}

struct cap_analog *cap_get_analog(struct cap *cap)
{
    if (CAP_TYPE_DIGITAL == cap->type)
        return NULL;
    return (struct cap_analog *) cap;
}

cap_digital_t *cap_get_digital(struct cap *cap)
{
    if (CAP_TYPE_DIGITAL == cap->type)
        return (struct cap_digital *) cap;
    return ((struct cap_analog *) cap)->dcap;
}
