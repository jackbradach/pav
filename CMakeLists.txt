cmake_minimum_required(VERSION 3.0)

project(pav)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(BINARY_NAME ${CMAKE_PROJECT_NAME})

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    include(CodeCoverage)
endif()

if (NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CTest)
endif()


include(assert_out_of_source_build)
assert_out_of_source_build(${CMAKE_PROJECT_NAME})

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Valid options are Debug (default) and Coverage.")

set(DEFAULT_C_FLAGS "-fopenmp -fdiagnostics-color -Wall -std=gnu11")
set(DEFAULT_CXX_FLAGS "-fopenmp -fdiagnostics-color -Wall -std=c++13")

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    # The CodeCoverage library takes care of adding the needed GCC options
    set(CMAKE_C_FLAGS "${DEFAULT_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
elseif (CMAKE_BUILD_TYPE STREQUAL "CoverageCpp")
        # The CodeCoverage library takes care of adding the needed GCC options
        set(CMAKE_C_FLAGS "${DEFAULT_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set (CMAKE_C_FLAGS "${DEFAULT_C_FLAGS} -g -O3")
    set (CMAKE_CXX_FLAGS "${DEFAULT_CMAKE_CXX_FLAGS} -g -O3")
else()
    set (CMAKE_C_FLAGS "${DEFAULT_C_FLAGS} -O3")
    set (CMAKE_CXX_FLAGS "${DEFAULT_C_FLAGS} -O3")
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/audio
)

# Find external libraries
find_package(Cairo REQUIRED)
include_directories(${CAIRO_INCLUDE_DIRS})
list(APPEND LIBS ${CAIRO_LIBRARIES})

#find_package(PLplot REQUIRED)
#include_directories(${PLplot_INCLUDE_DIR})
#list(APPEND LIBS ${PLplot_LIBRARY})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
list(APPEND LIBS ${ZLIB_LIBRARIES})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
list(APPEND LIBS ${SDL2_LIBRARY})

find_package(SDL2_mixer REQUIRED)
include_directories(${SDLMIXER_INCLUDE_DIR})
list(APPEND LIBS ${SDLMIXER_LIBRARY})

find_package(SDL2_ttf REQUIRED)
include_directories(${SDL2_TTF_INCLUDE_DIR})
list(APPEND LIBS ${SDL2_TTF_LIBRARY})

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
list(APPEND LIBS ${OPENGL_LIBRARIES})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
list(APPEND LIBS ${GLEW_LIBRARIES})

# GLUT may require libxmu-dev and libxi-dev
find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIRS})
list(APPEND LIBS ${GLUT_LIBRARY})

# Need Math library.
list(APPEND LIBS m)

# Add all the sub-builds
add_subdirectory(src)
add_subdirectory(src/gui)
add_subdirectory(src/gui/shaders)
add_subdirectory(src/gui/audio)
add_subdirectory(src/gui/fonts)

message("bindir: ${CMAKE_CURRENT_BINARY_DIR}")

# Build the main executable
add_executable(${BINARY_NAME}
    $<TARGET_OBJECTS:main>
    $<TARGET_OBJECTS:src>
    $<TARGET_OBJECTS:gui>
    $<TARGET_OBJECTS:res_audio>
    $<TARGET_OBJECTS:res_fonts>)
target_link_libraries(${BINARY_NAME} ${LIBS} shaders)



# Build the unit tests if Debug or Coverage
if (NOT CMAKE_BUILD_TYPE STREQUAL "Release")
if (NOT CMAKE_BUILD_TYPE STREQUAL "CoverageCpp")
    # Remove when GTest fixes their support for the 'TEST' operator
    cmake_policy(SET CMP0064 OLD)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
    add_subdirectory(tests)
    list(APPEND LIBS ${GTEST_BOTH_LIBRARIES})
    add_executable(
        ${BINARY_NAME}_tests
        EXCLUDE_FROM_ALL
        $<TARGET_OBJECTS:tests>
        $<TARGET_OBJECTS:src>
        $<TARGET_OBJECTS:gui>
        $<TARGET_OBJECTS:res_fonts>
    )
    target_link_libraries(${BINARY_NAME}_tests ${LIBS})
endif()
endif()

# Strip symbols for release
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET ${BINARY_NAME}
        POST_BUILD
        COMMAND strip -s ${EXECUTABLE_OUTPUT_PATH}/${BINARY_NAME}
    )
endif()


# Build documentation
if (0) # tie to build type!
include(build_naturaldocs)
build_naturaldocs(${CMAKE_PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/ref
    ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/nd
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


### Experimental C++ branch
if(CMAKE_BUILD_TYPE STREQUAL "CoverageCpp")
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
list(APPEND LIBS ${Boost_LIBRARIES})

find_package(Threads REQUIRED)
list(APPEND LIBS ${CMAKE_THREAD_LIBS_INIT})



add_subdirectory(src_cpp)
add_executable(${BINARY_NAME}_cpp
    $<TARGET_OBJECTS:pavcpp_main>
    $<TARGET_OBJECTS:pavcpp_src>
    $<TARGET_OBJECTS:src>
    $<TARGET_OBJECTS:gui>
    $<TARGET_OBJECTS:res_audio>
    $<TARGET_OBJECTS:res_fonts>)
target_link_libraries(${BINARY_NAME}_cpp ${LIBS} shaders)

# Build the unit tests if Debug or Coverage
if (CMAKE_BUILD_TYPE STREQUAL "CoverageCpp")
    include(CodeCoverage)
    # Remove when GTest fixes their support for the 'TEST' operator
    cmake_policy(SET CMP0064 OLD)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})

    add_subdirectory(tests_cpp)
    list(APPEND LIBS ${GTEST_BOTH_LIBRARIES})
    include_directories(src_cpp)

    add_executable(
        ${BINARY_NAME}_cpp_tests
        EXCLUDE_FROM_ALL
        $<TARGET_OBJECTS:tests_cpp>
        $<TARGET_OBJECTS:pavcpp_src>
    )
    target_link_libraries(${BINARY_NAME}_cpp_tests ${LIBS} pthread)
endif()
endif()
