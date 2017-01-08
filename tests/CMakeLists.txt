

set(TEST_SRCS
    tests.cpp
    test_adc.cpp
    test_audio.cpp
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

add_library(tests OBJECT ${TEST_SRCS})

add_custom_target(copy_captures
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/tests/captures/
            ${CMAKE_BINARY_DIR}/tests
)

add_test(NAME AnalogDecode
    COMMAND pav -l100 --decode ${CMAKE_BINARY_DIR}/tests/uart_analog_115200_50mHz.bin.gz
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_dependencies(check ${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_NAME}_tests tests copy_captures)

# Create a 'coverage' target and make sure it builds the binaries!
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    setup_target_for_coverage(coverage
        "${CMAKE_CTEST_COMMAND} --gtest_output=xml:${CMAKE_BINARY_DIR}/reports/pav_tests.xml"
        cov
    )
    add_dependencies(coverage check)
endif()

# Tarball up coverage results
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_SOURCE_INSTALLED_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}/cov/)
    set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-coverage")
    include(CPack)
    #add_dependencies(package, coverage)
endif()
