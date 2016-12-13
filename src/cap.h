#ifndef _CAP_H_
#define _CAP_H_


struct cap_analog {
    uint64_t nsamples;
    uint32_t nchannels;
    float period;
    struct adc_cal *cal;
    uint16_t **samples;
};

struct cap_digital {
    uint64_t nsamples;
    float period;
    uint32_t *samples;
};

#include "adc.h"

void cap_analog_free(struct cap_analog *acap);
void cap_digital_free(struct cap_digital *dcap);
void cap_analog_ch_copy(struct cap_analog *acap, uint8_t from, uint32_t to);
void cap_digital_ch_copy(struct cap_digital *dcap, uint8_t from, uint32_t to);

#endif
