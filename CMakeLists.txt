cmake_minimum_required(VERSION 3.4)

project(pav)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    include(CodeCoverage)
endif()


include(assert_out_of_source_build)
assert_out_of_source_build(${CMAKE_PROJECT_NAME})

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Valid options are Debug (default) and Coverage.")

set(DEFAULT_C_FLAGS "-fopenmp -fdiagnostics-color -Wall")

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    # The CodeCoverage library takes care of adding the needed GCC options
    set(CMAKE_C_FLAGS "${DEFAULT_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
    enable_testing()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set (CMAKE_C_FLAGS "${DEFAULT_C_FLAGS} -g -O3")
    set (CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
else()
    set (CMAKE_C_FLAGS "${DEFAULT_C_FLAGS} -O3")
    set (CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
endif()

enable_testing()

# Build the main source tree
add_subdirectory(src)

# Build unit tests
add_subdirectory(tests)

# Build documentation
if (0) # tie to build type!
include(build_naturaldocs)
build_naturaldocs(${CMAKE_PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/ref
    ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/nd
)
endif()
