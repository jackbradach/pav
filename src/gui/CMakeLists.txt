
set(GUI_SRC
    shaders.c
    display.c
    pav_gui.c
    gui.c
    gui_event.c
    gui_plot.c
    display.c
    views.c
)

add_library(gui OBJECT ${GUI_SRC})
