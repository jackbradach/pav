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

# Source containing 'main()'
set(SRC_MAIN
    pav.c
)

# The rest of the source gets built as a library so
# the unit tests can link against them.
set(SRC_LIBRARY
    adc.c
    cap.c
    file_utils.c
    pa_spi.c
    pa_usart.c
    pav_argp.c
    plot.c
    proto.c
    saleae.c
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
)

set(LIBRARY_NAME pavobj)
set(BINARY_NAME pav)

add_library(pavobj ${SRC_LIBRARY})

# Build gui
add_subdirectory(gui)
add_executable(${BINARY_NAME} ${SRC_MAIN} $<TARGET_OBJECTS:gui>)


target_link_libraries(${BINARY_NAME} ${LIBRARY_NAME}
    ${LIBS} shaders
    )


# Strip symbols for release
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET ${BINARY_NAME}
        POST_BUILD
        COMMAND strip -s ${EXECUTABLE_OUTPUT_PATH}/${BINARY_NAME}
    )
endif()



# Debian packager for releases
if (CMAKE_BUILD_TYPE STREQUAL "Release")

install(TARGETS ${BINARY_NAME}
        RUNTIME
        DESTINATION bin
)

set(EXAMPLES
    ${CMAKE_SOURCE_DIR}/tests/captures
)

install(DIRECTORY ${EXAMPLES}
    DESTINATION share/doc/${CMAKE_PROJECT_NAME}
    COMPONENT examples
)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jack Bradach <jack@bradach.net>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "zlib1g, libc6, libsdl2-2.0-0, libplplot12, libcairo2")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
include(CPack)
endif()
