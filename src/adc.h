#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>
#include <stdlib.h>


struct adc_cal {
    double vmax;
    double vmin;
};

#include "cap.h"

float adc_sample_to_voltage(uint16_t sample, struct adc_cal *cal);
uint16_t adc_voltage_to_sample(float voltage, struct adc_cal *cal);
int adc_ttl_convert(struct cap_analog *acap, struct cap_digital **dcap);
#endif
