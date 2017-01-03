#include <assert.h>
#include <math.h>
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

static void set_glviewport_for_view(view_t *v, unsigned idx);
static void draw_views(views_t *vl);
static void draw_view(view_t *v);


void display_gl_refresh(void)
{
    struct gui *g = gui_get_instance();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1) Draw UI
    // 2) Draw UI Data
    // 3) Draw views
    // 3a) Refresh views
//    control_refresh();
    // Pixels on the screen to be used for rendering
    draw_views(g->views);
    SDL_GL_SwapWindow(gui_get_window());
}


/* Draws all views in the views list */
static void draw_views(views_t *vl)
{
    view_t *v = views_first(vl);
    int vidx = 0;
    while (NULL != v) {
        views_refresh(v);
        set_glviewport_for_view(v, vidx);
        draw_view(v);
        v = views_next(v);
        vidx++;
    }
}

/* Draws a single view in the parent viewport. */
static void draw_view(view_t *v)
{
    cap_t *c = views_get_cap(v);
    float vmin, vmax, vrange;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    vmin = cap_get_analog_vmin(c);
    vmax = cap_get_analog_vmax(c);
    vrange = vmax - vmin;

    // XXX - 10% slop space so it's easier to view.  Make this a variable
    // elsewhere?
    glOrtho(0, 1.0, vmin - vrange/10.0, vmax + vrange/10.0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

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

/* Calls glViewport with a window appropriate for the specified data view.
 * This is where the window that each channel view is calculated.
 */
// XXX - view_t *v is not used yet, but it will eventually be needed
// XXX - to calculate the viewport (height-zoom, etc)
static void set_glviewport_for_view(view_t *v, unsigned idx)
{
    const double phi = (1.0 + sqrt(5.0)) / 2.0;
    float x0, y0;
    float view_width, view_height;
    int win_x, win_y;

    /* The view width is 4/5ths of the window and height can be 1/5th of the
     * screen.
     */
    // XXX - make this a little less jenkity.
    // XXX - also we're ignoring any borders around the screen and
    // XXX - individual channels that are needed to visually divide it up.
    // XXX - need scroll bars.
    gui_get_size(&win_x, &win_y);

    view_height = win_y / 4;
    view_width = win_x / phi;

    x0 = 0;
    y0 = win_y - ((idx * view_height) + view_height);
    glViewport(x0, y0, view_width, view_height);
}
