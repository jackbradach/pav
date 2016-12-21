#include <cmath>
#include <gtest/gtest.h>

#include "adc.h"

TEST(AdcTest, SampleToVoltage) {
    const uint16_t smax = 4095;
    struct adc_cal cal = {-10.0, 10.0};
    float v;

    /* With default scaling */
    for (uint16_t sample = 0; sample < smax; sample++) {
        static float last_v = -INFINITY;
        v = adc_sample_to_voltage(sample, NULL);
        ASSERT_TRUE(v > last_v);
        last_v = v;
    }

    /* With optional calibration struct */
    for (uint16_t sample = 0; sample < smax; sample++) {
        static float last_v = -INFINITY;
        v = adc_sample_to_voltage(sample, &cal);
        ASSERT_TRUE(v > last_v);
        last_v = v;
    }
}
