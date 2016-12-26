/* File: shaders.h
 *
 * OpenGL Shader manipulation
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
#ifndef _SHADERS_H_
#define _SHADERS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "queue.h"
#include "refcnt.h"

#ifdef __cplusplus
extern "C" {
#endif

enum shaders_avail {
    SHADER_DIM = 1,
    SHADER_BRIGHT,
    __SHADER_LAST
};
#define SHADER_MAX (__GUI_EVENT_LAST - 1)

typedef struct shader {
    TAILQ_ENTRY(shader) entry;
    struct refcnt rcnt;
} shader_t;
TAILQ_HEAD(shader_list, shader);
typedef struct shader_list shader_list_t;

GLint shader_compile_program(const char *vertex_src, const char *fragment_src);

#ifdef __cplusplus
}
#endif

#endif
