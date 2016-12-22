# Remove when GTest fixes their support for the 'TEST' operator
cmake_policy(SET CMP0064 OLD)

include_directories(${CMAKE_SOURCE_DIR}/src)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

find_package(PLplot REQUIRED)
include_directories(${PLplot_INCLUDE_DIR})
set(LIBS ${LIBS} ${PLplot_wxwidgets_LIBRARY} ${PLplot_LIBRARY})

find_package(Cairo REQUIRED)
include_directories(${CAIRO_INCLUDE_DIRS})
set(LIBS ${LIBS} ${CAIRO_LIBRARIES})

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
set(LIBS ${LIBS} ${ZLIB_LIBRARIES})

set(TEST_SRCS
    test_adc.cpp
    test_cap.cpp
    test_saleae.cpp
    test_pa_spi.cpp
    test_pa_usart.cpp
    test_plot.cpp
    test_proto.cpp
)

set(CTEST_OPTS "--build-run-dir ${CMAKE_CURRENT_BINARY_DIR}")
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} ${CTEST_OPTS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
foreach(SRC ${TEST_SRCS})
    get_filename_component(NAME ${SRC} NAME_WE)
endforeach(SRC)

GTEST_ADD_TESTS(${CMAKE_PROJECT_NAME}_tests "" ${TEST_SRCS})

add_executable(${CMAKE_PROJECT_NAME}_tests EXCLUDE_FROM_ALL ${TEST_SRCS})
add_dependencies(check ${CMAKE_PROJECT_NAME}_tests)
target_link_libraries(${CMAKE_PROJECT_NAME}_tests ${GTEST_BOTH_LIBRARIES} protoval ${LIBS})

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    setup_target_for_coverage(coverage ${CMAKE_PROJECT_NAME}_tests cov)
endif()


add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}_tests
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/tests/captures/
            ${EXECUTABLE_OUTPUT_PATH}
)
