#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int wire_to_double(double *pval, uint_least64_t onwire);
int wire_from_double(uint_least64_t *ponwire, double val);

#ifdef __cplusplus
}
#endif
