#include "common.h"
#include "internal/adapchol.h"
#include "internal/utils.h"
#include "backend/cpu/cpu.h"
#include "backend/fpga/fpga.h"
#include "internal/io.h"
#include "internal/cs_adap/cs_adap.h"
#include "internal/dispatcher.h"
#include "adapchol.h"

#include <cstring>
#include <cassert>
#include <vector>
#include <iostream>

namespace AdapChol {


    void AdapCholContext::fillP(bool *P, csi col) {
        // if current col don't have a parent, it does not need to have a P matrix
        if (symbol->parent[col] == -1) return;
        // rows of the current col (descendant)
        csi descRowBegin = L->p[col], descRowEnd = L->p[col + 1], descN = descRowEnd - descRowBegin;
        // rows of the parent col
        csi parent = symbol->parent[col];
        csi parRowBegin = L->p[parent], parRowEnd = L->p[parent + 1], parN = parRowEnd - parRowBegin;
        // Promise 1: the Ci in a col should be ordered, which is promised in the matrix L filling function.
        // Promise 2: descendant's first element below diag should be the same row as the parent's diag.
        // Assertion here to comfirm Promise 2 in debug mode.
        assert(descRowBegin + 1 < descRowEnd && L->i[descRowBegin + 1] == parent);
        // descendant has an element the same as the currently processing element in parent.
        // DESCENDANT  [1]     [4]
        // PARENT      [1] [2] [4]
        csi descRowIndex = descRowBegin + 1;
        csi indexP = 0;
        for (csi parRowIndex = parRowBegin; parRowIndex < parRowEnd; parRowIndex++) {
            while (descRowIndex < descRowEnd && L->i[descRowIndex] < L->i[parRowIndex])
                descRowIndex++;
            if (descRowIndex < descRowEnd && L->i[descRowIndex] == L->i[parRowIndex])
                P[indexP] = true;
            else
                P[indexP] = false;
            indexP++;
        }

    }

    void AdapCholContext::prepareIndexingPointers() {
        poolHead = poolTail = 0;
        pF = (double **) calloc(n, sizeof(double *));
        Fpool = (double **) calloc(n, sizeof(double *));
        pFn = (csi *) malloc(sizeof(csi) * n);
        csi pFnSorted[n];
        for (int i = 0; i < n; i++) {
            pFn[i] = pFnSorted[i] = symbol->cp[i + 1] - symbol->cp[i];
        }
        std::sort(pFnSorted, pFnSorted + n);
        maxFn = pFnSorted[n - 1];
        poolSplitStd = pFnSorted[(int) ((double) n * 0.6)];
#if defined(__x86_64__) || defined(_M_X64)
        cpuBackend->preProcessAMatrix(*this);

#else
        fpgaBackend->preProcessAMatrix(*this);

#endif

    }

    void AdapCholContext::run() {
        int csRelatedTime = 0, prepTime = 0, LRelatedTime = 0, transposeTime = 0,
                LTransTime = 0, preProcTime = 0, postProcTime = 0;
        TIMED_RUN_REGION_START(preProcTime)
        n = A->n;
        TIMED_RUN_REGION_START(csRelatedTime)
        symbol = adap_cs_schol(1, A);
        App = symbol->symp;
        AppL = symbol->AT;
        TIMED_RUN_REGION_END(csRelatedTime)

        TIMED_RUN_REGION_START(LRelatedTime)

#if defined(__x86_64__) || defined(_M_X64)
        cpuBackend->allocateAndFillL(*this);
#else
        fpgaBackend->allocateAndFillL(*this);
#endif
        TIMED_RUN_REGION_END(LRelatedTime)

        TIMED_RUN_REGION_START(prepTime)
        prepareIndexingPointers();
        TIMED_RUN_REGION_END(prepTime)
        TIMED_RUN_REGION_END(preProcTime)
//        for (int i = 0; i < n; i++) {
//            std::cout << symbol->cp[i + 1] - symbol->cp[i] << " ";
//            if (i % 10 == 0) std::cout << '\n';
//        }

        int dispatchTime = 0;

#if defined(__x86_64__) || defined(_M_X64)
//        for (int idx = 0; idx < n; idx++) {
//            csi col = symbol->post[idx];
//            cpuBackend->processAColumn(*this, col);
//        }
        Dispatcher dispatcher((int) n, symbol->parent);
        int res[n];
        int completed = 0;
        while (completed != n) {
            int count;
            TIMED_RUN_REGION_START(dispatchTime)
            count = dispatcher.dispatch(res, 4);
            TIMED_RUN_REGION_END(dispatchTime)
            cpuBackend->processColumns(*this, res, count);
            completed += count;
        }
#else

        Dispatcher dispatcher((int) n, symbol->parent);
        int res[n];
        int completed = 0;
        while (completed != n) {
            int count;
            TIMED_RUN_REGION_START(dispatchTime)
                count = dispatcher.dispatch(res, 4);
            TIMED_RUN_REGION_END(dispatchTime)
            fpgaBackend->processColumns(*this, res, count);
            completed += count;
        }
#endif
        TIMED_RUN_REGION_START(postProcTime)
#if defined(__x86_64__) || defined(_M_X64)
        cpuBackend->postProcessAMatrix(*this);

#else
        fpgaBackend->postProcessAMatrix(*this);

#endif
        TIMED_RUN_REGION_END(postProcTime)

        PERF_LOG("PreProcTime: %d", preProcTime)
        PERF_LOG("\tcsRelatedTime: %d", csRelatedTime)
        PERF_LOG("\tprepTime: %d", prepTime)
        PERF_LOG("\tLRelatedTime: %d", LRelatedTime)
        PERF_LOG("\ttransposeTime: %d", transposeTime)
        PERF_LOG("\tLtransposeTime: %d", LTransTime)
        PERF_LOG("\tdispatchTime: %d", dispatchTime)
        PERF_LOG("\tpostProcTime: %d", postProcTime)

        if (fpgaBackend)
            fpgaBackend->printStatistics();
    }

    int AdapCholContext::getMemPoolUsage() const {
        return poolHead;
    }

    cs *AdapCholContext::getResult() {
        return L;
    }

    void AdapCholContext::setA(cs *A_) {
        A = A_;
    }

    void AdapCholContext::setBackend(CPUBackend *cpuBackend_, FPGABackend *fpgaBackend_) {
        cpuBackend = cpuBackend_;
        fpgaBackend = fpgaBackend_;
    }


    double *AdapCholContext::getFrontal(int index) {
        return pF[index];
    }

    void AdapCholContext::postSolve(double *b) {
        // TODO: Free memory here
        auto *x = (double *) cs_malloc(n, sizeof(double));
        cs_ipvec(symbol->pinv, b, x, n);   /* x = P*b */
        cs_lsolve(L, x);           /* x = L\x */
        cs_ltsolve(L, x);          /* x = L'\x */
        cs_pvec(symbol->pinv, x, b, n);    /* b = P'*x */
    }
}