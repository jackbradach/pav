#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "cap.h"

// PLPLOT
#include "plplot/plplot.h"

void plot_analog_cap(struct cap_analog_new *acap, unsigned idx_start, unsigned idx_end)
{
    float vmin, vmax;
    uint16_t min, max;
    PLFLT *x, *y;
    PLFLT xmin, ymin, xmax, ymax;

    vmin = adc_sample_to_voltage(acap->sample_min, acap->cal);
    vmax = adc_sample_to_voltage(acap->sample_max, acap->cal);
    printf("Min: %d (%02fV)\n", min, vmin);
    printf("Max: %d (%02fV)\n", max, vmax);

    x = calloc(acap->nsamples, sizeof(PLFLT));
    y = calloc(acap->nsamples, sizeof(PLFLT));

    xmin = 0;
    ymin = vmin;
    xmax = (idx_end - idx_start);
    ymax = vmax;

    for (uint64_t i = idx_start, j = 0; i < idx_end; i++, j++) {
        // FIXME: the array needs to start at zero even if the indices do not!
        x[j] = (PLFLT) (idx_start + j);
        y[j] = adc_sample_to_voltage(acap->samples[i], acap->cal);
    }

    plsdev("wxwidgets");
    plinit();
    plenv(idx_start, idx_end, ymin, ymax + (ymax / 10), 0, 0);
    pllab("sample", "Voltage", "Analog Sample");
    plcol0(3);
    plline((idx_end - idx_start), x, y);
    plend();
    free(x);
    free(y);
}
