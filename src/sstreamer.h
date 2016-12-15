/* File: sstreamer.h
 *
 * Sample Streamer functions
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
#ifndef _SSTREAMER_H_
#define _SSTREAMER_H_

struct sstreamer_sub;
typedef void (*sink_analog)(struct sstreamer_sub *sub, uint16_t smpl, uint64_t len, struct adc_cal *cal);
typedef void (*sink_digital)(struct sstreamer_sub *sub, uint32_t *smpl, uint64_t len);

struct sstreamer_sub {
    void *ctx;
    sink_analog *sa;
    sink_digital *sd;
    uint32_t analog_mask;
    uint32_t digital_mask;
    uint32_t pa_flags;
};

struct sstreamer_ctx {
    struct cap_analog *acap;
    struct cap_digital *dcap;
    struct sstreamer_sub **subs;
    unsigned maxsubs;
    unsigned nsubs;
};

void sstreamer_ctx_alloc(struct sstreamer_ctx **uctx);
void sstreamer_ctx_free(struct sstreamer_ctx *ctx);
void sstreamer_sub_alloc(struct sstreamer_sub **usub, void *ctx);
void sstreamer_sub_free(struct sstreamer_sub *sub);
void sstreamer_add_sub(struct sstreamer_ctx *ctx, struct sstreamer_sub *sub);

#endif
