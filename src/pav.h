/* File: pav.h
 *
 * Protocol Analyzer Viewer - header
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
#ifndef _PAV_H_
#define _PAV_H_

#include <stdbool.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

enum pav_op {
    PAV_OP_INVALID = 0,
    PAV_OP_DECODE,
    PAV_OP_PLOTPNG,
    PAV_OP_GUI,
    PAV_OP_VERSION
};

struct pav_opts {
    FILE *fin;
    FILE *fout;
    char fin_name[512];
    char fout_name[512];
    enum pav_op op;
    unsigned nloops;
    uint64_t range_begin;
    uint64_t range_end;
    uint64_t duplicate;
    uint64_t skew_us;
    bool verbose;
};


#ifdef __cplusplus
}
#endif

#endif
