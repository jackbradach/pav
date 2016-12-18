# Do a bunch of rpath magic so that the end-user doesn't have to edit their
# ld configuration for anything linking against us.
function(rpath_fixup lib_install_path)
        SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
        SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
        SET(CMAKE_INSTALL_RPATH "${lib_install_path}")
        SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
        LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${lib_install_path}" isSystemDir)
        IF("${isSystemDir}" STREQUAL "-1")
           SET(CMAKE_INSTALL_RPATH "${lib_install_path}")
        ENDIF("${isSystemDir}" STREQUAL "-1")
endfunction(rpath_fixup)
