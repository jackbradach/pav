/* File: pav.c
 *
 * Protocol Analyzer Validation - main entry point.
 *
 * Author: Jack Bradach <jack@bradach.net>
 *
 * Copyright (C) 2016 Jack Bradach <jack@bradach.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <omp.h>
#include <time.h>
#include <unistd.h>

#include "pa_spi.h"
#include "pa_usart.h"
#include "adc.h"
#include "cap.h"
#include "saleae.h"
#include "sstreamer.h"
#include "plot.h"





void test_pa_spi(void)
{
    pa_spi_ctx_t *spi_ctx;
    clock_t ts_start, ts_end;
    double elapsed;

    uint64_t sample_count = 0;
    uint64_t decode_count = 0;
    struct cap_digital *dcap;
    int rc;

    /* Init SPI decoder and map physical channels to logical ones */
    pa_spi_ctx_init(&spi_ctx);
    pa_spi_ctx_map_mosi(spi_ctx, 0);
    pa_spi_ctx_map_miso(spi_ctx, 1);
    pa_spi_ctx_map_sclk(spi_ctx, 2);
    pa_spi_ctx_map_cs(spi_ctx, 3);
    pa_spi_ctx_set_flags(spi_ctx, SPI_FLAG_ENDIANESS);

    /* Map sample file into memory */
    rc = saleae_import_digital("16ch_quadspi_100mhz.bin", sizeof(uint32_t), 100.0E6, &dcap);
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

void test_adc_stream_single(void)
{
    pa_usart_ctx_t *usart_ctx;
    clock_t ts_start, ts_end;
    double elapsed;
    struct cap_analog *acap;
    struct cap_digital *dcap;
    int rc;

    uint64_t sample_count = 0;
    uint64_t decode_count = 0;

    /* Init USART decode, all defaults are fine. */
    pa_usart_ctx_init(&usart_ctx);
    pa_usart_ctx_map_data(usart_ctx, 0);
    pa_usart_ctx_set_freq(usart_ctx, 50.0E6);

    rc = saleae_import_analog("uart_analog_115200_50mHz.bin", NULL, &acap);
    if (rc) {
        printf("rc: %d\n", rc);
        abort();
    }
    ts_start = clock();
    adc_ttl_convert(acap, &dcap);
    uint8_t dout;

    /* Simulate 1 second of traffic by looping */
    uint64_t run_loop_target = (1/(dcap->nsamples * dcap->period));
    for (uint64_t loop = 0; loop < run_loop_target; loop++) {
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
    }
    ts_end = clock();
    printf("\n");
    pa_usart_ctx_cleanup(usart_ctx);
    elapsed = (double)(ts_end - ts_start) / CLOCKS_PER_SEC;
    printf("Time Simulated: 1.0 seconds\n");
    printf("Real Time: %f seconds\n", elapsed);
    printf("Samples processed: %'lu (%'lu samples/s)\n", sample_count, (unsigned long) (sample_count/elapsed));
    printf("Decode count: %'lu (%'lu bytes/s)\n", decode_count, (unsigned long) (decode_count/elapsed));
}

void test_adc_stream_multi(uint8_t nstreams)
{
    pa_usart_ctx_t *usart_ctx[nstreams];
    clock_t ts_start, ts_end;
    double elapsed;
    struct cap_analog *acap[nstreams];
    struct cap_digital *dcap[nstreams];
    int rc;

    uint64_t sample_count = 0;
    uint64_t decode_count = 0;
    uint64_t run_loop_target = 0;


    /* Init USART decode, all defaults are fine. */
    for (int i = 0; i < nstreams; i++) {
        pa_usart_ctx_init(&usart_ctx[i]);
        pa_usart_ctx_map_data(usart_ctx[i], 0);
        pa_usart_ctx_set_freq(usart_ctx[i], 50.0E6);
        saleae_import_analog("uart_analog_115200_50mHz.bin", NULL, &acap[i]);
    }

    ts_start = clock();

    for (int i = 0; i < nstreams; i++) {
        adc_ttl_convert(acap[i], &dcap[i]);

    }
    run_loop_target = (1/(dcap[0]->nsamples * dcap[0]->period));
    run_loop_target = 5;
    /* Simulate 1 second of traffic by looping */
    //for (uint64_t loop = 0; loop < run_loop_target; loop++) {
        //for (unsigned long i = 0; i < dcap[0]->nsamples; i++)
        for (unsigned long i = 0; i < 10; i++)
        {
            #pragma omp parallel for default(shared)
            for (int s = 0; s < nstreams; s++) {
                int rc;
                uint8_t dout;

                rc = pa_usart_stream(usart_ctx[s], dcap[s]->samples[i], &dout);
                if (PA_SPI_DATA_VALID == rc) {
                    #pragma omp atomic
                    decode_count++;
                }
                #pragma omp atomic
                sample_count++;
            }
        }
    //}
    ts_end = clock();
    printf("\n");

    for (int i = 0; i < nstreams; i++) {
        pa_usart_ctx_cleanup(usart_ctx[i]);
    }
    elapsed = (double)(ts_end - ts_start) / CLOCKS_PER_SEC;
    printf("Time Simulated: %f seconds\n", 1.0);
    printf("Real Time: %f seconds\n", elapsed);
    printf("Analog streams decoded: %d\n", nstreams);
    printf("Samples processed: %'lu (%'lu samples/s)\n", sample_count, (unsigned long) (sample_count/elapsed));
    printf("Decode count: %'lu (%'lu bytes/s)\n", decode_count, (unsigned long) (decode_count/elapsed));
}

void test_pa_usart(void)
{
    pa_usart_ctx_t *usart_ctx;
    clock_t ts_start, ts_end;
    double elapsed;

    uint64_t sample_count = 0;
    uint64_t decode_count = 0;
    struct cap_digital *dcap;
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

void test_sstreamer(void)
{
    struct sstreamer_ctx *streamer;
    struct cap_digital *dcap;
    const float capture_freq = 500.0E6;
    uint64_t sample_count = 0;
    uint64_t decode_count = 0;
    int rc;

    sstreamer_ctx_alloc(&streamer);

    /* Load digital samples */
    rc = saleae_import_digital("uart_digital_115200_500mHz.bin", sizeof(uint32_t), capture_freq, &dcap);
    assert (0 == rc);

    // Clone channels
    cap_digital_ch_copy(dcap, 1, 0xFFFE);
    streamer->dcap = dcap;

    // create decoder contexts
    for (int i = 0; i < 4; i++) {

        struct sstreamer_sub *sub;
        pa_usart_ctx_t *usart_ctx;
        pa_usart_ctx_init(&usart_ctx);
        pa_usart_ctx_map_data(usart_ctx, i);
        pa_usart_ctx_set_freq(usart_ctx, capture_freq);
        sstreamer_sub_alloc(&sub, usart_ctx);
        sstreamer_add_sub(streamer, sub);
    }


    //for (uint64_t idx = 0; idx < )

    sstreamer_ctx_free(streamer);
    //streamer_addsub();

}


double analog_find_rise_time(uint16_t *buf)
{

}


void test_analog(void)
{
    clock_t ts_start, ts_end;
    double elapsed;
    struct cap_bundle *cbun;
    struct cap_analog_new *acap;
    int rc;

    uint64_t sample_count = 0;
    uint64_t decode_count = 0;

    rc = saleae_import_analog_new("uart_analog_115200_50mHz.bin", NULL, &cbun);
    if (rc) {
        printf("rc: %d\n", rc);
        abort();
    }

    // XXX - Testing!
    acap = cbun->acaps[0];

    ts_start = clock();



    plot_analog_cap(acap, 10000, 20000);
    ts_end = clock();
    elapsed = (double)(ts_end - ts_start) / CLOCKS_PER_SEC;
    printf("Real Time: %f seconds\n", elapsed);
    cap_bundle_free(cbun);
}



int main(void)
{
    /* Make printf add an appropriate thousand's delimiter, based on locale */
    setlocale(LC_NUMERIC, "");
//    test_pa_spi();
//    test_pa_usart();
//    printf("\n");
//    test_adc_stream_single();
//    test_sstreamer();
    test_analog();


    return 0;
}
