
set(GUI_SRC
    audio.c
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
