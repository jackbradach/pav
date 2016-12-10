#ifndef _ANALOG_PA_
#define _ANALOG_PA_

struct adc_cal {
    double vmax;
    double vmin;
};
float adc_apply_scale(uint16_t sample, struct adc_cal *cal);
int adc_ch_samples(void *abuf, uint8_t ch, uint16_t **ch_buf);
void adc_print_saleae_hdr(void *abuf);
uint16_t adc_volt_to_raw(float voltage);
#endif
