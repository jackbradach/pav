set(LIBNAME "res_fonts")

set(SRC_FONTS
    C64_Pro.ttf
)

foreach(SRC ${SRC_FONTS})
    get_filename_component(BASE ${SRC} NAME_WE)
    get_filename_component(EXT ${SRC} EXT)
    string(SUBSTRING ${EXT} 1 4 EXT)
    set(NAME res_fonts_${BASE}_${EXT})
    # Create C source containing binary blob
    add_custom_command(
        OUTPUT ${NAME}.c
        PRE_BUILD
        COMMAND echo "#include <stddef.h>\\n" > ${NAME}.c
        COMMAND echo "char ${NAME}[] = {" >> ${NAME}.c
        COMMAND hexdump -ve "1/1 \"0x%02x,\"" < ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} >> ${NAME}.c
        COMMAND echo "\\n};\\n" >> ${NAME}.c
        COMMAND echo "size_t ${NAME}_size = sizeof(${NAME});" >> ${NAME}.c
        DEPENDS ${SRC}
        VERBATIM
    )

    set_source_files_properties(
        ${NAME}.c PROPERTIES
        GENERATED TRUE
    )

    list(APPEND RES_FONTS_GEN "${NAME}.c")

    # Create C header to refer to binary blob
    set(OUTNAME ${CMAKE_BINARY_DIR}/${NAME}.h)
    add_custom_command(
        OUTPUT ${NAME}.h
        PRE_BUILD
        COMMAND echo "#ifndef _${NAME}_H_" > ${OUTNAME}
        COMMAND echo "#define _${NAME}_H_" >> ${OUTNAME}
        COMMAND echo "\\n#include <stddef.h>\\n" >> ${OUTNAME}
        COMMAND echo "extern char ${NAME}[];" >> ${OUTNAME}
        COMMAND echo "extern size_t ${NAME}_size;" >> ${OUTNAME}
        COMMAND echo "\\n#endif\\n" >> ${OUTNAME}
        DEPENDS ${SRC}
        VERBATIM
    )

    set_source_files_properties(
        ${NAME}.h PROPERTIES
        GENERATED TRUE
    )

    list(APPEND RES_FONTS_GEN "${NAME}.h")
endforeach()


add_library(res_fonts OBJECT ${RES_FONTS_GEN})
#add_custom_target(res_font_dummy ALL DEPENDS res_fonts)
