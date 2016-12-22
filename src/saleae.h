#ifndef _SALEAE_H_
#define _SALEAE_H_

#ifdef __cplusplus
extern "C" {
#endif

void saleae_import_analog(const char *src_file, struct cap_bundle **new_bundle);
int saleae_import_digital(const char *cap_file, size_t sample_width, float freq, struct cap_digital **new_dcap);
void saleae_import_analog_new(FILE *fp, struct cap_bundle **new_bundle);
#ifdef __cplusplus
}
#endif

#endif
