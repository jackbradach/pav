#include <assert.h>
#include <stdint.h>

#include "cap.h"

void cap_analog_free(struct cap_analog *acap)
{
    if (NULL == acap)
        return;

    for(int i = 0; i < acap->nchannels; i++) {
        free(acap->samples[i]);
    }

    if (acap->samples)
        free(acap->samples);
    free(acap);
}

void cap_digital_free(struct cap_digital *dcap)
{
    if (dcap->samples)
        free(dcap->samples);
    free(dcap);
}

/* Function: cap_analog_ch_copy
 *
 * Replicates the data from a source channel to targets, specified by mask.
 * Each channel consumes uint16_t * nsamples of memory.
 * TODO - 2016/12/12 - jbradach - make this more efficient by setting up aliases
 * TODO - that map logical channels to physical ones.
 *
 * Parameters:
 *      from - channel ID for source data_valid
 *      to - 32-bit mask specifying destination channels.  This mask must not
 *          include the source channel!
 */
void cap_analog_ch_copy(struct cap_analog *acap, uint8_t from, uint32_t to)
{
    assert((from < 32) && (to < 32));
    assert((1 << from) & to);

    // TODO - finish me!
    abort();
    for (uint64_t i = 0; i < acap->nsamples; i++) {
    }
}

/* Function: cap_digital_ch_copy
 *
 * Replicates the data from a source channel to targets, specified by mask.
 * This is "free" memory-wise, as internally digital captures are saved
 * as a 32-bit mask representing the channels.
 *
 * Parameters:
 *      from - channel ID for source data_valid
 *      to - 32-bit mask specifying destination channels.  This mask must not
 *          include the source channel!
 */
void cap_digital_ch_copy(struct cap_digital *dcap, uint8_t from, uint32_t to)
{
    assert((from < 32) && (to < 32));
    assert((1 << from) & to);

    for (uint64_t i = 0; i < dcap->nsamples; i++) {
        uint32_t tmp = dcap->samples[i] &= ~to;

        if (dcap->samples[i] & (1 << from)) {
            tmp |= dcap->samples[i];
        }
        dcap->samples[i] = tmp;
    }
}
