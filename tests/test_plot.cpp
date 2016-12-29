#include <cstdio>
#include <gtest/gtest.h>
#include <SDL2/SDL.h>
#include "plplot/plplot.h"
#include "cairo/cairo.h"

#include "cap.h"
#include "plot.h"
#include "saleae.h"
#include "queue.h"


TEST(Plot, PlotLifeCycle) {
    plot_t *p, *p2;
    unsigned refcnt;

    p = plot_create();
    refcnt = 1;

    /* Check that structure was allocated  */
    ASSERT_TRUE(NULL != p);

    /* Make sure reference counting works */
    ASSERT_EQ(refcnt, plot_getref(p));
    p2 = plot_addref(p);
    refcnt++;
    ASSERT_EQ(p, p2);
    ASSERT_EQ(refcnt, plot_getref(p));

    /* Teardown */
    plot_dropref(p);
    refcnt--;
    ASSERT_EQ(refcnt, plot_getref(p));
    plot_dropref(p);

    /* Call with invalid pointers (should be no-op) */
    ASSERT_EQ(NULL, plot_addref(NULL));
    ASSERT_EQ(0, plot_getref(NULL));
    plot_dropref(NULL);
}

TEST(Plot, AccessorsMutators) {
    const char gold_xlabel[] = "Test x-axis label";
    const char gold_ylabel[] = "Test y-axis label";
    const char gold_title[] = "Test title label";
    plot_t *p;

    p = plot_create();

    plot_set_xlabel(p, gold_xlabel);
    plot_set_ylabel(p, gold_ylabel);
    plot_set_title(p, gold_title);

    ASSERT_STREQ(plot_get_xlabel(p), gold_xlabel);
    ASSERT_STREQ(plot_get_ylabel(p), gold_ylabel);
    ASSERT_STREQ(plot_get_title(p), gold_title);

    plot_dropref(p);
}

TEST(Plot, PlotFromView) {
    cap_bundle_t *b;
    views_t *vl;
    view_t *v;
    plot_t *p;
    const char test_file[] = "uart_analog_115200_50mHz.bin.gz";
    FILE *fp = fopen(test_file, "rb");

    saleae_import_analog(fp, &b);
    views_populate_from_bundle(b, &vl);
    cap_bundle_dropref(b);

    v = views_first(vl);
    plot_from_view(v, &p);

    plot_dropref(p);
}

TEST(Plot, DISABLED_PlotToCairoSurface) {
    cap_bundle_t *b;
    views_t *vl;
    view_t *v;
    plot_t *pl;
    cairo_surface_t *cs;
    const char test_file[] = "uart_analog_115200_50mHz.bin.gz";
    FILE *fp = fopen(test_file, "rb");

    saleae_import_analog(fp, &b);
    views_populate_from_bundle(b, &vl);
    cap_bundle_dropref(b);

    v = views_first(vl);
    plot_from_view(v, &pl);

    cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 640, 480);
    plot_to_cairo_surface(pl, cs);

    plot_dropref(pl);
    cairo_surface_destroy(cs);
    fclose(fp);
}
