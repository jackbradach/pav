#ifndef SALEAE_H
#define SALEAE_H

int saleae_import_analog(const char *cap_file, const char *cal_file, struct cap_analog **new_acap);
int saleae_import_digital(const char *cap_file, size_t sample_width, float freq, struct cap_digital **new_dcap);
int saleae_import_analog_new(const char *cap_file, const char *cal_file, struct cap_bundle **new_bundle);
#endif
