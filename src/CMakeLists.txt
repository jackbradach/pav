# Source containing 'main()'
set(MAIN
    pav.c
)

set(SRC
    adc.c
    cap.c
    pa_spi.c
    pa_usart.c
    pav_argp.c
    plot.c
    proto.c
    saleae.c
)

if(${WIN32})
list(APPEND SRC file_utils_win32.c)
else()
list(APPEND SRC file_utils_linux.c)
endif()

add_library(main OBJECT ${MAIN})
add_library(src OBJECT ${SRC})
