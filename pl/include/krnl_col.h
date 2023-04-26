#pragma once
#include <hls_burst_maxi.h>

void krnl_proc_col(hls::burst_maxi<double> descF, bool *P, double* parF,
                   int descFn, int parFn);
