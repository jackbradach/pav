find_package(Cairo REQUIRED)
include_directories(${CAIRO_INCLUDE_DIRS})
set(LIBS ${LIBS} ${CAIRO_LIBRARIES})

find_package(PLplot REQUIRED)
include_directories(${PLplot_INCLUDE_DIR})
set(LIBS ${LIBS} ${PLplot_wxwidgets_LIBRARY} ${PLplot_LIBRARY})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
set(LIBS ${LIBS} ${ZLIB_LIBRARIES})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
set(LIBS ${LIBS} ${SDL2_LIBRARY})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
set(LIBS ${LIBS} ${GLEW_LIBRARIES})

# Need Math library.
list(APPEND LIBS m)


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

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
)

add_library(gui OBJECT ${GUI_SRC})

# Build shader blobs
add_subdirectory(shaders)
