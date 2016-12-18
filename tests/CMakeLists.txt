# Remove when GTest fixes their support for the 'TEST' operator
cmake_policy(SET CMP0064 OLD)
find_package(GTest REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${GTEST_INCLUDE_DIRS})

set(TEST_SRCS
    test_cap.cpp
    test_saleae.cpp
)
foreach(SRC ${TEST_SRCS})
    get_filename_component(NAME ${SRC} NAME_WE)
    add_executable(${NAME} ${SRC})
    if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
        setup_target_for_coverage(${NAME}_cov ${NAME} cov)
    endif()
    target_link_libraries("${NAME}" ${GTEST_BOTH_LIBRARIES} protoval)
    GTEST_ADD_TESTS(${NAME} "" ${SRC})
endforeach(SRC)
