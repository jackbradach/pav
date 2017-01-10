#ifndef PAV_CAPTURE_H
#define PAV_CAPTURE_H

#include <array>
#include <string>

#include <boost/scoped_array.hpp>

/*! Container for signal captures.
 *
 * These are created via importing from file or converting another
 * capture format.
 */
struct Capture {

public:
    Capture(void) = delete; // Unneeded default constructor
    Capture(uint16_t *data, size_t length, double period); // Actual constructor
    std::string &note(void);
    void note(std::string &note);
    unsigned long num_samples(void);
    unsigned analog_sample_min(void);
    unsigned analog_sample_max(void);
    double sample_period(void);
    double sample_frequency(void);
    unsigned physical_channel_id(void);

private:
    std::string _note;
    unsigned long _num_samples;
    unsigned _physical_channel_id;

    double _period;
    unsigned _analog_sample_min;
    unsigned _analog_sample_max;


    boost::scoped_array<unsigned short> _analog_samples;
    boost::scoped_array<uint8_t> _digital_samples;

    void Adc(uint16_t v_lo, uint16_t v_hi);
    void AdcTTL(void);
    void UpdateAnalogMinMax(void);


    // Need Calibration struct for min/max
    // Need proto bucket
};

#endif
