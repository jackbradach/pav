#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GL/glut.h>
#include <SDL2/SDL.h>

#include "display.h"
#include "gui.h"
#include "views.h"

static void draw_view(view_t *v);


void display_gl_refresh(void)
{
    struct gui *g = gui_get_instance();
    view_t *v;
    int w, h;
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gui_get_size(&w, &h);
    printf("wxh = %dx%d\n", w,h);

    // Pixels on the screen to be used for rendering
    glViewport(128, 0, w-128, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Ortho-look for view


    v = views_first(g->views);
    while (NULL != v) {
        cap_t *c = views_get_cap(v);
        size_t vlen;

        views_refresh(v);

        vlen = views_get_width(v);
        printf("Drawing view, len: %'lu\n", vlen);

        glOrtho(0, 1.0, cap_get_analog_vmin(c), cap_get_analog_vmax(c), -1, 1);
        glMatrixMode(GL_MODELVIEW);
        draw_view(v);

        v = views_next(v);
    }
    SDL_GL_SwapWindow(gui_get_window());
    printf("Scene!\n");
}

static void draw_view(view_t *v)
{
    glPushMatrix();
    glLoadIdentity();
    //glTranslatef(5, -1, 0);

    glLineWidth(views_get_line_width(v));
    glColor3f(views_get_red(v), views_get_green(v), views_get_blue(v));
    // glUseProgram(v->shader)
    glBindBuffer(GL_ARRAY_BUFFER, views_get_vbo_vertices(v));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, views_get_vbo_idx(v));
    glDrawElements(GL_LINE_STRIP, views_get_width(v), GL_UNSIGNED_INT, NULL);
    glDisableVertexAttribArray(0);
    glUseProgram(0);
    glPopMatrix();

}
