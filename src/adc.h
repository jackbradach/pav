#ifndef _ANALOG_PA_
#define _ANALOG_PA_

#include <stdint.h>
#include <stdlib.h>

struct adc_cal {
    double vmax;
    double vmin;
};

struct analog_cap {
    uint64_t nsamples;
    uint32_t nchannels;
    float period;
    struct adc_cal *cal;
    uint16_t **samples;
};

struct digital_cap {
    uint64_t nsamples;
    float period;
    uint32_t *samples;
};

float adc_apply_scale(uint16_t sample, struct adc_cal *cal);
int adc_ch_samples(void *abuf, uint8_t ch, uint16_t **ch_buf);
void adc_print_saleae_hdr(void *abuf);
uint16_t adc_volt_to_raw(float voltage, struct adc_cal *cal);
int adc_ttl_convert(struct analog_cap *acap, struct digital_cap **dcap);
#endif
