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

void display_gl_view_update(view_t *v)
{


}

void display_gl_triangle(void)
{
    glClearColor(0.0f, 0.2f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, 1024, 768);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.5f, 0.0f, 0.5f);

    glBegin(GL_TRIANGLES);
        glVertex3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, 0.0f);
    glEnd();
    glTranslatef(3.0f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
        glVertex3f(-1.0f, 1.0f, 0.0f);
        glVertex3f(1.0f, 1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f);
    glEnd();
    SDL_GL_SwapWindow(gui_get_window());
}

void display_gl_refresh(void)
{
    struct gui *g = gui_get_instance();
    view_t *v;
    int idx = 0;

    glClearColor(0.0f, 0.2f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, 1024, 768);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLineWidth(2.0);
    glColor3b(0xff, 0, 0);
    v = views_first(g->views);
    while (NULL != v) {
        views_refresh(v);

        // glUseProgram(v->shader)
        glBindBuffer(GL_ARRAY_BUFFER, views_get_vbo(v));
        glEnableVertexAttribArray(views_get_vbo(v));
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0 ,0);
        glDrawArrays(GL_LINE_STRIP, 0, views_get_width(v));
        glDisableVertexAttribArray(0);
        glUseProgram(0);

        idx++;
        v = views_next(v);
    }
    SDL_GL_SwapWindow(gui_get_window());

}
