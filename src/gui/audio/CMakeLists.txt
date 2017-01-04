set(LIBNAME "res_shaders")
set(OBJCOPY_FMT elf64-x86-64)
set(OBJCOPY_BINARCH i386)
set(OBJCOPY_PREFIX res_shader)

set(SRC_AUDIO
    sfx_click.ogg
)

foreach(SRC ${SRC_AUDIO})
    get_filename_component(BASE ${SRC} NAME_WE)
    get_filename_component(EXT ${SRC} EXT)
    string(SUBSTRING ${EXT} 1 4 EXT)
    set(NAME res_audio_${BASE}_${EXT})

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

    # Create C header to refer to binary blob
    add_custom_command(
        OUTPUT ${NAME}.h
        PRE_BUILD
        COMMAND echo "#ifndef _${NAME}_H_" > ${NAME}.h
        COMMAND echo "#define _${NAME}_H_" >> ${NAME}.h
        COMMAND echo "\\n#include <stddef.h>\\n" >> ${NAME}.h
        COMMAND echo "extern char ${NAME}[];" >> ${NAME}.h
        COMMAND echo "extern size_t ${NAME}_size;" >> ${NAME}.h
        COMMAND echo "\\n#endif\\n" >> ${NAME}.h
        DEPENDS ${SRC}
        VERBATIM
    )
    set_source_files_properties(
        ${NAME}.c PROPERTIES
        GENERATED TRUE
    )
    set_source_files_properties(
        ${NAME}.h PROPERTIES
        GENERATED TRUE
    )
    list(APPEND RES_AUDIO_GEN "${NAME}.c")
endforeach()


add_library(res_audio OBJECT ${RES_AUDIO_GEN})

#add_custom_target(res ALL DEPENDS res_audio)
