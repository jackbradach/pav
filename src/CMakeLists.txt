# Source containing 'main()'
set(MAIN
    pav.c
)

set(SRC
    adc.c
    cap.c
    file_utils.c
    pa_spi.c
    pa_usart.c
    pav_argp.c
    plot.c
    proto.c
    saleae.c
)

add_library(main OBJECT ${MAIN})
add_library(src OBJECT ${SRC})
