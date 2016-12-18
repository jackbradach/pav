# Remove when GTest fixes their support for the 'TEST' operator
cmake_policy(SET CMP0064 OLD)
find_package(GTest REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${GTEST_INCLUDE_DIRS})

set(TEST_SRCS
    test_cap.cpp
    test_saleae.cpp
)


add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
foreach(SRC ${TEST_SRCS})
    get_filename_component(NAME ${SRC} NAME_WE)
endforeach(SRC)

GTEST_ADD_TESTS(${CMAKE_PROJECT_NAME}_tests "" ${TEST_SRCS})

add_executable(${CMAKE_PROJECT_NAME}_tests EXCLUDE_FROM_ALL ${TEST_SRCS})
add_dependencies(check ${CMAKE_PROJECT_NAME}_tests)
target_link_libraries(${CMAKE_PROJECT_NAME}_tests ${GTEST_BOTH_LIBRARIES} protoval)

if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    setup_target_for_coverage(coverage ${CMAKE_PROJECT_NAME}_tests cov)
endif()
