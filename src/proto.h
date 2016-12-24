/* File: proto.h
 *
 * Protocol Analyzer Viewer - protocol decode containers (headers)
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
#ifndef _PROTO_H_
#define _PROTO_H_

#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct proto proto_t;
typedef struct proto_dframe proto_dframe_t;

#define PROTO_MAX_NOTE_LEN 64

/* Accessors for the sample index and frame */
void *proto_dframe_udata(struct proto_dframe *df);
uint64_t proto_dframe_idx(proto_dframe_t *df);
int proto_dframe_type(struct proto_dframe *df);
proto_dframe_t *proto_dframe_first(proto_t *pr);
proto_dframe_t *proto_dframe_next(proto_dframe_t *df);
proto_dframe_t *proto_dframe_last(proto_t *pr);

proto_t *proto_create(void);
proto_t *proto_addref(proto_t *pr);
unsigned proto_getref(proto_t *pr);
void proto_dropref(proto_t *pr);

void proto_set_note(proto_t *pr, const char *s);
const char *proto_get_note(proto_t *pr);
void proto_set_period(proto_t *pr, float t);
float proto_get_period(proto_t *pr);
uint64_t proto_get_nframes(proto_t *pr);

void proto_add_dframe(proto_t *pr, uint64_t idx, int type, void *udata);

typedef void (*proto_sink_t)(proto_dframe_t *df, void *udata);
void proto_foreach(proto_t *pr, proto_sink_t *sink);



#ifdef __cplusplus
}
#endif

#endif
