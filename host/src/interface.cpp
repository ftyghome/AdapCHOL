#include "common.h"
#include "internal/adapchol.h"
#include "backend/cpu/cpu.h"
#include "backend/fpga/fpga.h"
#include "adapchol.h"


namespace AdapChol {
    AdapCholContext *allocateContext() {
        return new AdapCholContext();
    }

    CPUBackend *allocateCPUBackend() {
        return new CPUBackend();
    }

    FPGABackend *allocateFPGABackend(const std::string &binaryFile, int cus_) {
#if defined(__x86_64__) || defined(_M_X64)
        return nullptr;
#else
        return new FPGABackend(binaryFile, cus_);
#endif
    }

    cs *loadSparse(FILE *file) {
        return cs_compress(cs_load(file));
    }

    void setA(AdapCholContext *context, cs *A_) {
        context->setA(A_);
    }

    void setBackend(AdapCholContext *context, CPUBackend *cpuBackend_, FPGABackend *fpgaBackend_) {
        context->setBackend(cpuBackend_, fpgaBackend_);
    }

    void run(AdapCholContext *context) {
        context->run();
    }

    cs *getResult(AdapCholContext *context) {
        return context->getResult();
    }

    int getMemPoolUsage(AdapCholContext *context) {
        return context->getMemPoolUsage();
    }

    std::vector<int> getDispatchPerfData(AdapCholContext *context) {
        return context->getDispatchPerfData();
    }

    cs *getL(csn *numeric) {
        return numeric->L;
    }

    css *cs_schol(int order, const cs *A) {
        return ::cs_schol(order, A);
    }

    csn *cs_chol(const cs *A, const css *S) {
        return ::cs_chol(A, S);
    }

    cs *allocateSparse(int order, int nzmax) {
        return cs_spalloc(order, order, nzmax, 1, 0);
    }

    int *getSparseP(cs *matrix) {
        return matrix->p;
    }

    int *getSparseI(cs *matrix) {
        return matrix->i;
    }

    double *getSparseX(cs *matrix) {
        return matrix->x;
    }

    int getOrder(cs *matrix) {
        return matrix->n;
    }

    int getNzNum(cs *matrix) {
        return matrix->nz;
    }

    void cs_cholsol(const cs *matrix, double *b) {
        ::cs_cholsol(1, matrix, b);
    }

    void postSolve(AdapCholContext *context, double *b) {
        context->postSolve(b);
    }
}