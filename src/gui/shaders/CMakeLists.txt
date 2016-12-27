set(LIBNAME "res_shaders")
set(OBJCOPY_FMT elf64-x86-64)
set(OBJCOPY_PREFIX res_shader)


# the unit tests can link against them.
set(SRC_SHADERS
    test.vert
    test.frag
)

foreach(SHADER ${SRC_SHADERS})
get_filename_component(BASE ${SHADER} NAME_WE)
get_filename_component(EXT ${SHADER} EXT)
string(SUBSTRING ${EXT} 1 4 EXT)

set(SHADER_SRC ${SHADER})
add_custom_command(
    OUTPUT ${SHADER_SRC}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/src/gui/shaders/${SHADER_SRC}
            ${CMAKE_BINARY_DIR}/shaders/${SHADER_SRC}
)

set(SHADER_OBJ res_${BASE}_${EXT}.o)
set(RES_SHADER_PATH ${CMAKE_BINARY_DIR}/src/gui/shaders)
set(RES_SHADER_SRC_PATH ${CMAKE_BINARY_DIR}/shaders)

add_custom_command(
    OUTPUT ${RES_SHADER_PATH}/${SHADER_OBJ}
    COMMAND ${CMAKE_OBJCOPY}
            --redefine-sym _binary_${BASE}_${EXT}_start=${OBJCOPY_PREFIX}_${BASE}_${EXT}_start
            --redefine-sym _binary_${BASE}_${EXT}_end=${OBJCOPY_PREFIX}_${BASE}_${EXT}_end
            --redefine-sym _binary_${BASE}_${EXT}_size=${OBJCOPY_PREFIX}_${BASE}_${EXT}_size
            --input binary
            --output ${OBJCOPY_FMT}
            ${SHADER_SRC}
            ${RES_SHADER_PATH}/${SHADER_OBJ}
    DEPENDS ${RES_SHADER_PATH}/${SHADER_SRC}
    WORKING_DIRECTORY ${RES_SHADER_SRC_PATH}
)

list(APPEND RES_OBJS res_${BASE}_${EXT}.o)
endforeach()

set_source_files_properties(
    ${RES_OBJS} PROPERTIES
    EXTERNAL_OBJECT TRUE
    GENERATED TRUE
)

add_library(${LIBNAME} STATIC ${RES_OBJS})
set_target_properties(${LIBNAME} PROPERTIES LINKER_LANGUAGE C)
set(RES_SHADERS_LIBRARY ${RES_SHADER_PATH}/lib${LIBNAME}.a PARENT_SCOPE)
