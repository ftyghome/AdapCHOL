#pragma once

#include "common.h"
#include <cstdint>
#include <string>



#ifndef _CS_H
struct cs_sparse;
struct cs_symbolic;
struct cs_numeric;

typedef cs_sparse cs;
typedef cs_symbolic css;
typedef cs_numeric csn;
#endif


namespace AdapChol {

    class Backend;

    class CPUBackend;

    class FPGABackend;

    class AdapCholContext;

    AdapCholContext *allocateContext();

    void setA(AdapCholContext *context, cs *A_);

    void setBackend(AdapCholContext *context, CPUBackend *cpuBackend_, FPGABackend *fpgaBackend_);

    void run(AdapCholContext *context);

    CPUBackend *allocateCPUBackend();

    FPGABackend *allocateFPGABackend(const std::string &binaryFile, int cus_);

    cs *getResult(AdapCholContext *context);

    cs *loadSparse(FILE *file);

    int getMemPoolUsage(AdapCholContext *context);

    void dumpFormalResult(std::ofstream &stream, cs *mat);

    css *cs_schol(int order, const cs *A);

    csn *cs_chol(const cs *A, const css *S);

    cs* getL(csn* numeric);

    cs *allocateSparse(int order, int nzmax);

    int *getSparseP(cs *matrix);

    int *getSparseI(cs *matrix);

    double *getSparseX(cs *matrix);
}


