#ifndef _SALEAE_H_
#define _SALEAE_H_

#ifdef __cplusplus
extern "C" {
#endif

void saleae_import_analog(FILE *fp, struct cap_bundle **new_bundle);
int saleae_import_digital(FILE *fp, size_t sample_width, float freq, cap_digital_t **dcap);

#ifdef __cplusplus
}
#endif

#endif
