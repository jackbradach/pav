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

# Source containing 'main()'
set(SRC_MAIN
    pav.c
)

# The rest of the source gets built as a library so
# the unit tests can link against them.
set(SRC_LIBRARY
    adc.c
    cap.c
    gui.c
    pa_spi.c
    pa_usart.c
    pav_argp.c
    plot.c
    proto.c
    saleae.c
    sstreamer.c
)

set(LIBRARY_NAME protoval)
set(BINARY_NAME pav)

add_library(${LIBRARY_NAME} ${SRC_LIBRARY})
add_executable(${BINARY_NAME} ${SRC_MAIN})
target_link_libraries(${BINARY_NAME} ${LIBRARY_NAME} ${LIBS})
