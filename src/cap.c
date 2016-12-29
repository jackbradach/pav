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
#include "proto.h"

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
    uint16_t analog_min;
    uint16_t analog_max;
    uint16_t *analog;
    adc_cal_t *analog_cal;
    uint8_t *digital;
    proto_t *proto;
    struct refcnt rcnt;
};
TAILQ_HEAD(cap_list, cap);

struct cap_bundle {
    struct refcnt rcnt;
    struct cap_list head;
    unsigned len;
};

/* Static prototypes - these get automatically called by
 * the reference counter.
 */
static void cap_free(const struct refcnt *ref);
static void cap_bundle_free(const struct refcnt *ref);

/* Populates the analog min/max fields of a capture */
void cap_update_analog_minmax(struct cap *c)
{
    uint16_t min = 4095;
    uint16_t max = 0;

    for (uint64_t i = 0; i < c->nsamples; i++) {
        uint16_t sample = c->analog[i];
        if (sample < min) {
            min = sample;
        } else if (sample > max) {
            max = sample;
        }
    }

    c->analog_min = min;
    c->analog_max = max;
}

void cap_analog_adc_ttl(struct cap *c)
{
    const uint16_t ttl_low = adc_voltage_to_sample(0.8f, c->analog_cal);
    const uint16_t ttl_high = adc_voltage_to_sample(2.0f, c->analog_cal);
    cap_analog_adc(c, ttl_low, ttl_high);
}

void cap_analog_adc(struct cap *c, uint16_t v_lo, uint16_t v_hi)
{
    uint8_t *digital = calloc(c->nsamples, sizeof(uint8_t));

    for (uint64_t i = 0; i < c->nsamples; i++) {
        static uint8_t adc = 0;
        /* Digital samples only change when crossing the voltage
         * thresholds.
         */
        uint16_t ch_sample = c->analog[i];
        if (ch_sample <= v_lo) {
            adc = 0;
        } else if (ch_sample >= v_hi) {
            adc = 1;
        }
        digital[i] = adc;
    }

    /* Shred any existing digital capture */
    if (c->digital) {
        free(c->digital);
    }

    c->digital = digital;
}


void cap_clone_to_bundle(struct cap_bundle *bun, struct cap *src, unsigned nloops, unsigned skew_us)
{
    struct cap *dst;
    unsigned src_len = src->nsamples;
    unsigned dst_len = src_len * nloops;
    unsigned skew = (skew_us * 1E-6) / src->period;

    dst = cap_create(dst_len);
    dst->period = src->period;
    dst->analog_min = src->analog_min;
    dst->analog_max = src->analog_max;
    dst->analog_cal = src->analog_cal;

    for (unsigned i = 0; i < dst_len; i++) {
        dst->analog[i] = src->analog[(i + skew) % src_len];
    }
    cap_analog_adc_ttl(dst);
    cap_bundle_add(bun, dst);
}

/* Function: cap_create
 *
 * Allocates and initializes a new capture structure
 *
 * Parameters:
 *  len - Number of samples this capture will contain
 *
 * Returns:
 *  pointer to new cap_analog with refcnt = 1.
 *
 * See Also:
 *  <cap_dropref>
 */
struct cap *cap_create(size_t len)
{
    struct cap *c;
    c = calloc(1, sizeof(struct cap));
    c->rcnt = (struct refcnt) { cap_free, 1 };
    c->nsamples = len;
    c->analog = calloc(len, sizeof(uint16_t));
    c->digital = calloc(len, sizeof(uint8_t));
    return c;
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
struct cap *cap_addref(struct cap *c)
{
    if (c == NULL)
        return NULL;

    refcnt_inc(&c->rcnt);

    return c;
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

    if (c->parent) {
        cap_dropref(c->parent);
        c->parent = NULL;
    }

    refcnt_dec(&c->rcnt);
}

/* Function: cap_nref
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
unsigned cap_nref(cap_t *c)
{
    if (NULL == c)
        return 0;
    return c->rcnt.count;
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
    cap_t *c =
        container_of(ref, cap_t, rcnt);

    /* Clean up child structures */
    if (c->analog)
        free(c->analog);

    if (c->digital)
        free(c->digital);

    if (c->proto)
        proto_dropref(c->proto);

    free(c);
}

/* Function: cap_bundle_create
 *
 * Allocates and initializes a new capture bundle.
 *
 * Returns:
 *  pointer to new capture bundle.
 *
 * See Also:
 *  <cap_bundle_dropref>
 */
struct cap_bundle *cap_bundle_create(void)
{
    struct cap_bundle *b;
    b = calloc(1, sizeof(struct cap_bundle));
    b->rcnt = (struct refcnt) { cap_bundle_free, 1 };

    /* Set up an empty capture list */
    TAILQ_INIT(&b->head);

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

/* Function: cap_bundle_nref
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
unsigned cap_bundle_nref(struct cap_bundle *b)
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

unsigned cap_bundle_len(struct cap_bundle *b)
{
    return b->len;
}

uint64_t cap_get_nsamples(struct cap *c)
{
    return c->nsamples;
}

uint16_t cap_get_analog(struct cap *c, uint64_t idx)
{
    return c->analog[idx];
}

float cap_get_analog_voltage(struct cap *c, uint64_t idx)
{
    return adc_sample_to_voltage(c->analog[idx], c->analog_cal);
}

void cap_set_analog(struct cap *c, uint64_t idx, uint16_t sample)
{
    c->analog[idx] = sample;
}

uint8_t cap_get_digital(struct cap *c, uint64_t idx)
{
    return c->digital[idx];
}

void cap_set_digital(struct cap *c, uint64_t idx, uint8_t sample)
{
    c->digital[idx] = sample;
}

uint16_t cap_get_analog_min(struct cap *c)
{
    return c->analog_min;
}

uint16_t cap_get_analog_max(struct cap *c)
{
    return c->analog_max;
}

uint8_t cap_get_physical_ch(struct cap *c)
{
    return c->physical_ch;
}

void cap_set_physical_ch(struct cap *c, uint8_t ch)
{
    c->physical_ch = ch;
}

float cap_get_period(struct cap *c)
{
    return c->period;
}

void cap_set_period(struct cap *c, float t)
{
    c->period = t;
}

void cap_bundle_add(struct cap_bundle *b, struct cap *c)
{
    TAILQ_INSERT_TAIL(&b->head, c, entry);
    b->len++;
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

adc_cal_t *cap_get_analog_cal(struct cap *c)
{
    return c->analog_cal;
}

void cap_set_analog_cal(struct cap *c, float vmin, float vmax)
{
    if (c->analog_cal)
        free(c->analog_cal);
    c->analog_cal = adc_cal_create(vmin, vmax);
}

uint64_t cap_get_offset(cap_t *cap)
{
    return cap->offset;
}

void cap_set_offset(struct cap *c, uint64_t offset)
{
    c->offset = offset;
}

const char *cap_get_note(struct cap *c)
{
    return (const char *) &c->note;
}

void cap_set_note(struct cap *c, const char *s)
{
    strncpy(c->note, s, CAP_MAX_NOTE_LEN);
}

#if 0
/* Function: cap_create_subcap
 *
 * Creates a subcapture, which is a structure containing a
 * slice of the samples in the parent structure.  It should be
 * treated identically to a normal capture structure and needs to
 * be dropref'd when done for garbage collection.
 */
cap_t *cap_create_subcap(struct cap *src, int64_t begin, int64_t end)
{
    struct cap *dst, *top;
    int64_t abs_begin = begin + src->offset;
    int64_t abs_end = end + src->offset;

    if (abs_begin < 0)
        abs_begin = 0;

    top = cap_get_top(src);

    if (abs_end >= top->nsamples)
        abs_end = top->nsamples - 1;

    dst = cap_create(src->nsamples);

    /* Subcaptures get an offset pointer into the top-level captures's
     * data but they report their own sample lengths.
     */
    if (src->analog) {
        dst->analog = top->analog + (abs_begin * sizeof(uint16_t));

        /* Note that we don't update the min/max
         * on a subcap; it should be the same as the parent!
         */
        dst->analog_min = src->analog_min;
        dst->analog_max = src->analog_max;
        dst->analog_cal = src->analog_cal;
    }

    if (src->digital) {
        dst->digital = top->digital + abs_begin;
    }

    /* Copy the rest of the capture structure data */
    dst->parent = cap_addref(src);
    cap_set_note(dst, cap_get_note(src));
    dst->physical_ch = src->physical_ch;
    dst->period = src->period;
    dst->offset = abs_begin;

    return dst;
}


int cap_get_nparents(struct cap *c)
{
    struct cap *top = c;
    int nparents = 0;

    while (NULL != top->parent) {
        nparents++;
        top = top->parent;
    }
    return nparents;
}
#endif

uint64_t cap_next_edge(struct cap *c, uint64_t from)
{
    uint64_t next = from + 1;

    for (int i = 0; i < 2 && next < c->nsamples; next++) {
        if (c->digital[from] != c->digital[next]) {
            i++;
        }
    }

    if (next > c->nsamples)
        next = c->nsamples;

    return next;
}

uint64_t cap_prev_edge(struct cap *c, uint64_t from)
{
    int64_t prev;

    for (prev = from - 2; prev > 0; prev--) {
        if (c->digital[from] != c->digital[prev])
            break;
    }

    if (prev < 0)
        prev = 0;

    return prev;
}
