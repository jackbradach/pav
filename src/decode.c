#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "spi_decode.h"

void *mmap_file(const char *filename, size_t *length)
{
    int fd;
    struct stat st;
    void *addr;

    fd = open(filename, O_RDONLY);
    fstat(fd, &st);

    addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    *length = st.st_size;
    return addr;
}

int main(void)
{
    spi_decode_ctx_t *spi_ctx;
    clock_t ts_start, ts_end;
    double elapsed;

    uint32_t *sbuf;
    uint64_t sample_count = 0;
    uint64_t decode_count = 0;
    size_t sbuf_len;


    /* Init SPI decoder and map physical channels to logical ones */
    spi_decode_ctx_init(&spi_ctx);
    spi_decode_ctx_map_mosi(spi_ctx, 0);
    spi_decode_ctx_map_miso(spi_ctx, 1);
    spi_decode_ctx_map_sclk(spi_ctx, 2);
    spi_decode_ctx_map_cs(spi_ctx, 3);
    spi_decode_ctx_set_flags(spi_ctx, SPI_FLAG_ENDIANESS);

    /* Map sample file into memory */
    sbuf = mmap_file("../16ch_quadspi_100mhz.bin", &sbuf_len);

    ts_start = clock();
//    while (sample_count < 1000000000ULL) {
        for (unsigned long i = 0; i < (sbuf_len / sizeof(uint32_t)); i++)
        {
            uint8_t dout, din;
            int rc;

            rc = spi_decode_stream(spi_ctx, sbuf[i], &dout, &din);
            if (SPI_DECODE_DATA_VALID == rc) {
                decode_count++;
            }
            sample_count++;
        }
//    }
    ts_end = clock();
    spi_decode_ctx_cleanup(spi_ctx);
    elapsed = (double)(ts_end - ts_start) / CLOCKS_PER_SEC;
    printf("Time elapsed: %f seconds\n", elapsed);
    printf("Samples processed: %lu (%lu samples/s)\n", sample_count, (unsigned long) (sample_count/elapsed));
    printf("Decode count: %lu (%lu bytes/s)\n", decode_count, (unsigned long) (decode_count/elapsed));

    munmap(sbuf, sbuf_len);

    return 0;
}
