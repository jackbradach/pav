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

    glClearColor(0.0f, 0.2f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, 1024, 768);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double w = glutGet( GLUT_WINDOW_WIDTH );
    double h = glutGet( GLUT_WINDOW_HEIGHT );
    glOrtho( 0, w, 0, h, -1, 1 );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glLineWidth(1.0);
    glColor3f(1.0, 0, 0);
    v = views_first(g->views);
    while (NULL != v) {
        size_t vlen;

        views_refresh(v);

        vlen = views_get_width(v);

        // glUseProgram(v->shader)
        glBindBuffer(GL_ARRAY_BUFFER, views_get_vbo_vertices(v));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, views_get_vbo_idx(v));
        glDrawElements(GL_LINE_STRIP, views_get_width(v), GL_UNSIGNED_INT, NULL);
        glDisableVertexAttribArray(0);
        glUseProgram(0);

        v = views_next(v);
    }
    SDL_GL_SwapWindow(gui_get_window());

}
