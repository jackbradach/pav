#include <string>

#include "capture.h"

using std::copy;
using std::string;
using boost::scoped_array;


Capture::Capture(uint16_t *data, size_t length, double period)
: _num_samples(length),
  _period(period)
{
    uint16_t *buf = new uint16_t[length];
    copy(data, data + length, buf);
    _analog_samples.reset(buf);

    /* Wark! */
}

string &Capture::note(void)
{
    return this->_note;
}

void Capture::note(string &note)
{
    this->_note = note;
}

unsigned long Capture::num_samples(void)
{
    return this->_num_samples;
}

unsigned Capture::analog_sample_min(void)
{
    return this->_analog_sample_min;
}

unsigned Capture::analog_sample_max(void)
{
    return this->_analog_sample_max;
}

double Capture::sample_period(void)
{
    return this->_period;
}

double Capture::sample_frequency(void)
{
    return 1.0 / this->_period;
}

unsigned Capture::physical_channel_id(void)
{
    return this->_physical_channel_id;
}

void Capture::UpdateAnalogMinMax(void)
{
    unsigned min = -1;
    unsigned max = 0;

    for (uint64_t i = 0; this->_num_samples; i++) {
        uint16_t sample = this->_analog_samples[i];
        if (sample < min) {
            min = sample;
        } else if (sample > max) {
            max = sample;
        }
    }

    this->_analog_sample_min = min;
    this->_analog_sample_max = max;
}

/*!
 * Does an Analog->Digital Conversion using TTL voltage level.
 */
void Capture::AdcTTL(void)
{
//    const uint16_t ttl_low = adc_voltage_to_sample(0.8f, c->analog_cal);
//    const uint16_t ttl_high = adc_voltage_to_sample(2.0f, c->analog_cal);
//    cap_analog_adc(c, ttl_low, ttl_high);
}

#if 0
/*!
 * Does an Analog->Digital Conversion using the specified logic
 * level voltage thresholds.
 */
void Capture::Adc(uint16_t v_lo, uint16_t v_hi)
{
    uint8_t *digital = calloc(this->num_samples, sizeof(uint8_t));

    for (uint64_t i = 0; i < this->num_samples; i++) {
        static uint8_t adc = 0;
        /* Digital samples only change when crossing the voltage
         * thresholds.
         */
        uint16_t ch_sample = this->analog_samples[i];
        if (ch_sample <= v_lo) {
            adc = 0;
        } else if (ch_sample >= v_hi) {
            adc = 1;
        }
        digital[i] = adc;
    }

    /* Shred any existing digital capture */
    if (this->digital) {
        free(this->digital);
    }

    this->digital = digital;
}
#endif
#if 0
void cap_clone_to_bundle(struct cap_bundle *bun, struct cap *src, unsigned nloops, unsigned skew_us)
{
    struct cap *dst;
    unsigned src_len = src->nsamples;
    unsigned dst_len = src_len * nloops;
    unsigned skew = (skew_us * 1E-6) / src->period;

    dst = cap_create(dst_len);
    dst->period = src->period;
    dst->analog_min = src->analog_min;
    dst->analog_max = src->analog_max;
    dst->analog_cal = src->analog_cal;


    for (unsigned i = 0; i < dst_len; i++) {
        dst->analog[i] = src->analog[(i + skew) % src_len];
    }
    cap_analog_adc_ttl(dst);
    cap_bundle_add(bun, dst);
}
#endif
