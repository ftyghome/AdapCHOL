#pragma once

#include <hls_burst_maxi.h>

void krnl_proc_col(double *descF, bool *P, double *parF, double *L, int descLOffset,
                   int descFn, int parFn);
