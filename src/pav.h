#ifndef _PAV_H_
#define _PAV_H_

#include <stdbool.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

enum pav_op {
    PAV_OP_INVALID = 0,
    PAV_OP_DECODE,
    PAV_OP_PLOTPNG,
    PAV_OP_VERSION
};

struct pav_opts {
    FILE *fin;
    FILE *fout;
    char fin_name[64];
    char fout_name[64];
    enum pav_op op;
    unsigned loops;
    bool verbose;
};


#ifdef __cplusplus
}
#endif

#endif
