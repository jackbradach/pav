/* File: sstreamer.h
 *
 * Sample Streamer functions
 */
#ifndef _SSTREAMER_H_
#define _SSTREAMER_H_

struct sstreamer_sub;
typedef void (*sink_analog)(struct sstreamer_sub *sub, uint16_t smpl, uint64_t len, struct adc_cal *cal);
typedef void (*sink_digital)(struct sstreamer_sub *sub, uint32_t *smpl, uint64_t len);

struct sstreamer_sub {
    void *ctx;
    sink_analog *sa;
    sink_digital *sd;
    uint32_t analog_mask;
    uint32_t digital_mask;
    uint32_t pa_flags;
};

struct sstreamer_ctx {
    struct cap_analog *acap;
    struct cap_digital *dcap;
    struct sstreamer_sub **subs;
    unsigned maxsubs;
    unsigned nsubs;
};

void sstreamer_ctx_alloc(struct sstreamer_ctx **uctx);
void sstreamer_ctx_free(struct sstreamer_ctx *ctx);
int sstreamer_sub_alloc(struct sstreamer_sub **usub, void *ctx);
void sstreamer_sub_free(struct sstreamer_sub *sub);
void sstreamer_add_sub(struct sstreamer_ctx *ctx, struct sstreamer_sub *sub);

#endif
