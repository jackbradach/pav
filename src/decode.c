#include <stdio.h>
#include <time.h>

#include "spi_decode.h"

int main(void)
{
    FILE *fp;
    spi_decode_ctx_t *spi_ctx;
    clock_t ts_start, ts_end;
    double elapsed;

    uint64_t sample_count = 0;
    uint64_t decode_count = 0;


    /* Init SPI decoder and map physical channels to logical ones */
    spi_decode_ctx_init(&spi_ctx);
    spi_decode_ctx_map_mosi(spi_ctx, 1);
    spi_decode_ctx_map_miso(spi_ctx, 2);
    spi_decode_ctx_map_sclk(spi_ctx, 3);
    spi_decode_ctx_map_cs(spi_ctx, 4);
    spi_decode_ctx_set_flags(spi_ctx, SPI_FLAG_ENDIANESS);

    fp = fopen("../6ch_async_spi_100mhz.bin", "rb");
    ts_start = clock();
    while (!feof(fp)) {
        uint8_t dout, din;
        int rc;
        int raw = fgetc(fp);

        rc = spi_decode_stream(spi_ctx, raw, &dout, &din);
        if (SPI_DECODE_DATA_VALID == rc) {
            decode_count++;
        }
        sample_count++;
    }
    ts_end = clock();
    fclose(fp);

    spi_decode_ctx_cleanup(spi_ctx);
    elapsed = (double)(ts_end - ts_start) / CLOCKS_PER_SEC;
    printf("Time elapsed: %f seconds\n", elapsed);
    printf("Samples processed: %lu (%lu samples/s)\n", sample_count, (unsigned long) (sample_count/elapsed));
    printf("Decode count: %lu (%lu bytes/s)\n", decode_count, (unsigned long) (decode_count/elapsed));

    return 0;
}
