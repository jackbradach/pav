function(assert_out_of_source_build project_name)
        get_filename_component(src "${CMAKE_SOURCE_DIR}" REALPATH)
        get_filename_component(bin "${CMAKE_BINARY_DIR}" REALPATH)

        # If they aren't in the source directory, we're good.
        if(NOT "${src}" STREQUAL "${bin}")
                return()
        endif()
        message("
                  . ,
                 - * -
                  ' )
                  _(_
                ,'   `.
               /       \\
              |  = F =  |
               \\       /
                `.___,'
        ")

        message("#############################################################")
        message("# Don't configure and build ${project_name} in its source directory!")
        message("# Instead, clean up and re-run cmake from another directory,")
        message("# Example:")
        message("# $ rm -rf CMakeFiles CMakeCache.txt")
        message("# $ mkdir build")
        message("# $ cd build")
        message("# $ cmake ..")
        message("# $ make")
        message("#############################################################")

        message(FATAL_ERROR "Failed out-of-source build check")
endfunction(assert_out_of_source_build)
