#include <cmath>
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/scoped_array.hpp>
#include <gtest/gtest.h>

#include "capture.h"

using std::string;

TEST(CaptureTest, CaptureLifecycle) {
    string gold_note = "Test Note";
    unsigned gold_num_samples = 50;
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> dist(0, 4096);
    uint16_t *gold_analog_samples = new unsigned short[gold_num_samples];


    for (int i = 0; i < gold_num_samples; i++) {
        gold_analog_samples[i] = dist(rng);
    }


    Capture *c = new Capture(gold_analog_samples, gold_num_samples, 100e6);
    ASSERT_TRUE(NULL != c);

    c->note(gold_note);
    ASSERT_EQ(c->note(), gold_note);

    ASSERT_EQ(c->num_samples(), gold_num_samples);

    delete c;
}
