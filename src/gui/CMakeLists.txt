# XXX - eventually move this to the resource library (it's so it
# XXX - pick up generated files)
include_directories(${CMAKE_BINARY_DIR})

set(GUI_SRC
    audio.c
    control.c
    shaders.c
    display.c
    display_gl.c
    pav_gui.c
    gui.c
    gui_event.c
    gui_plot.c
    display.c
    views.c
)

add_library(gui OBJECT ${GUI_SRC})
