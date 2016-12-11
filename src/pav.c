#include <assert.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "pa_spi.h"
#include "pa_usart.h"
#include "adc.h"
#include "saleae.h"

void *mmap_file(const char *filename, size_t *length)
{
    int fd;
    struct stat st;
    void *addr;

    // TODO - 2016/12/08 - jbradach - add more robust error handling here.
    fd = open(filename, O_RDONLY);
    assert(fd > 0);

    fstat(fd, &st);

    addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    *length = st.st_size;
    return addr;
}

void test_pa_spi(void)
{
    pa_spi_ctx_t *spi_ctx;
    clock_t ts_start, ts_end;
    double elapsed;

    uint64_t sample_count = 0;
    uint64_t decode_count = 0;
    struct digital_cap *dcap;
    int rc;

    /* Init SPI decoder and map physical channels to logical ones */
    pa_spi_ctx_init(&spi_ctx);
    pa_spi_ctx_map_mosi(spi_ctx, 0);
    pa_spi_ctx_map_miso(spi_ctx, 1);
    pa_spi_ctx_map_sclk(spi_ctx, 2);
    pa_spi_ctx_map_cs(spi_ctx, 3);
    pa_spi_ctx_set_flags(spi_ctx, SPI_FLAG_ENDIANESS);

    /* Map sample file into memory */
    rc = saleae_import_digital("16ch_quadspi_100mhz.bin", 100.0E6, sizeof(uint32_t), &dcap);
    if (rc) {
        printf("rc: %d\n", rc);
    }

    ts_start = clock();
    for (unsigned long i = 0; i < dcap->nsamples; i++)
    {
        uint8_t dout, din;
        int rc;

        rc = pa_spi_stream(spi_ctx, dcap->samples[i], &dout, &din);
        if (PA_SPI_DATA_VALID == rc) {
            decode_count++;
        }
        sample_count++;
    }
    ts_end = clock();
    pa_spi_ctx_cleanup(spi_ctx);
    elapsed = (double)(ts_end - ts_start) / CLOCKS_PER_SEC;
    printf("Time elapsed: %f seconds\n", elapsed);
    printf("Samples processed: %'lu (%'lu samples/s)\n", sample_count, (unsigned long) (sample_count/elapsed));
    printf("Decode count: %'lu (%'lu bytes/s)\n", decode_count, (unsigned long) (decode_count/elapsed));
}

void test_adc(void)
{
    struct analog_cap *acap;
    int rc;

    rc = saleae_import_analog("3ch_analog_raw_50Mhz.bin", NULL, &acap);

    if (rc) {
        printf("rc: %d\n", rc);
    }

}

void test_pa_usart(void)
{
    pa_usart_ctx_t *usart_ctx;
    clock_t ts_start, ts_end;
    double elapsed;

    uint64_t sample_count = 0;
    uint64_t decode_count = 0;
    struct digital_cap *dcap;
    int rc;

    /* Init USART decode, all defaults are fine. */
    pa_usart_ctx_init(&usart_ctx);
    pa_usart_ctx_map_data(usart_ctx, 0);

    /* Map sample file into memory */
    rc = saleae_import_digital("uart_digital_115200_500mHz.bin", sizeof(uint32_t), 500.0E6, &dcap);
    if (rc) {
        printf("rc: %d\n", rc);
    }

    pa_usart_ctx_set_freq(usart_ctx, 500.0E6);

    ts_start = clock();
    for (unsigned long i = 0; i < dcap->nsamples; i++)
    {
        uint8_t dout;
        int rc;

        rc = pa_usart_stream(usart_ctx, dcap->samples[i], &dout);
        if (PA_SPI_DATA_VALID == rc) {
            decode_count++;
        }
        sample_count++;
    }
    ts_end = clock();
    printf("\n");
    pa_usart_ctx_cleanup(usart_ctx);
    elapsed = (double)(ts_end - ts_start) / CLOCKS_PER_SEC;
    printf("Time elapsed: %f seconds\n", elapsed);
    printf("Samples processed: %'lu (%'lu samples/s)\n", sample_count, (unsigned long) (sample_count/elapsed));
    printf("Decode count: %'lu (%'lu bytes/s)\n", decode_count, (unsigned long) (decode_count/elapsed));
}


int main(void)
{
    /* Make printf add an appropriate thousand's delimiter, based on locale */
    setlocale(LC_NUMERIC, "");
//    test_pa_spi();
    test_pa_usart();
    printf("\n");
//    test_adc();

    return 0;
}
