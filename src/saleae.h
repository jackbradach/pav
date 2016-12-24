/* File: saleae.h
 *
 * Protocol Analyzer Viewer - capture file import (Saleae format) (headers)
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
#ifndef _SALEAE_H_
#define _SALEAE_H_

#ifdef __cplusplus
extern "C" {
#endif

void saleae_import_analog(FILE *fp, struct cap_bundle **new_bundle);
int saleae_import_digital(FILE *fp, size_t sample_width, float freq, cap_digital_t **dcap);

#ifdef __cplusplus
}
#endif

#endif
