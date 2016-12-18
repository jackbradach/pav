/* File: refcnt.h
 *
 * Simple reference counting for shared structs.
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
#ifndef _REFCNT_H_
#define _REFCNT_H_

#include <stddef.h>

#define container_of(ptr, type, member) \
    ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

struct refcnt {
    void (*free)(const struct refcnt *ref);
    int count;
};

static inline void refcnt_inc(const struct refcnt *ref)
{
    __sync_add_and_fetch((int *) &ref->count, 1);
}

static inline void refcnt_dec(const struct refcnt *ref)
{
    int v;
    v = __sync_sub_and_fetch((int *) &ref->count, 1);
    if (0 == v)
        ref->free(ref);
}

#endif
