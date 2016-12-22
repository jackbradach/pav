#include <cmath>
#include <gtest/gtest.h>

#include "adc.h"

TEST(AdcTest, CalLifecycle) {
    const float GOLD_VMIN = -10.0;
    const float GOLD_VMAX = 10.0;
    adc_cal_t *cal = adc_cal_create(GOLD_VMIN, GOLD_VMAX);

    ASSERT_FLOAT_EQ(adc_cal_get_vmin(cal), GOLD_VMIN);
    ASSERT_FLOAT_EQ(adc_cal_get_vmax(cal), GOLD_VMAX);

    free(cal);
}

TEST(AdcTest, SampleToVoltage) {
    const uint16_t smax = 4095;
    adc_cal_t *cal = adc_cal_create(-10.0, 10.0);
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
        v = adc_sample_to_voltage(sample, cal);
        ASSERT_TRUE(v > last_v);
        last_v = v;
    }
}
