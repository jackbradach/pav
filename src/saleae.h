#ifndef SALEAE_H
#define SALEAE_H

int saleae_import_analog(const char *cap_file, const char *cal_file, struct analog_cap **new_acap);
int saleae_import_digital(const char *cap_file, size_t sample_width, struct digital_cap **new_dcap);

#endif
