#include "adapchol.h"
#include "utils.h"
#include "backend/cpu/cpu.h"
#include "internal/io.h"
#include "internal/cs_adap/cs_adap.h"
#include <cstring>
#include <cassert>
#include <vector>
#include <iostream>

namespace AdapChol {

    void AdapCholContext::allocateAndFillL() {

    }


    void AdapCholContext::fillP(csi col) {
        bool *P = publicP;
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
        pF = (double **) calloc(n, sizeof(double *));
        Fpool = (double **) calloc(n, sizeof(double *));
        pFn = (csi *) malloc(sizeof(csi) * n);
        for (int i = 0; i < n; i++) {
            pFn[i] = symbol->cp[i + 1] - symbol->cp[i];
            if (pFn[i] > maxFn)
                maxFn = pFn[i];
        }
#if defined(__x86_64__) || defined(_M_X64)
        cpuBackend->preProcessAMatrix(*this);

#else
        fpgaBackend->preProcessAMatrix(*this);

#endif

    }

    void AdapCholContext::run() {
        int64_t csRelatedTime = 0, prepTime = 0, LRelatedTime = 0, transposeTime = 0, LTransTime = 0;
        auto preProcTime = timedRun([&] {
            n = A->n;
            csRelatedTime = timedRun([&] {
                symbol = adap_cs_schol(1, A);
                App = symbol->symp;
                AppL = symbol->AT;
            });

            LRelatedTime = timedRun([&] {
#if defined(__x86_64__) || defined(_M_X64)
                cpuBackend->allocateAndFillL(*this);
#else
                fpgaBackend->allocateAndFillL(*this);
#endif

            });
            prepTime = timedRun([&] {
                prepareIndexingPointers();
            });
        });
        std::cerr << "PreProcTime: " << preProcTime << "\n\tIncluding:"
                  << "\n\tcsRelatedTime: " << csRelatedTime
                  << "\n\tprepTime: " << prepTime
                  << "\n\tLRelatedTime: " << LRelatedTime
                  << "\n\ttransposeTime: " << transposeTime
                  << "\n\tLtransposeTime: " << LTransTime
                  << std::endl;
        for (int idx = 0; idx < n; idx++) {
            csi col = symbol->post[idx];
#if defined(__x86_64__) || defined(_M_X64)
            cpuBackend->processAColumn(*this, col);
#else
            fpgaBackend->processAColumn(*this, col);

#endif
        }
#if defined(__x86_64__) || defined(_M_X64)
        cpuBackend->postProcessAMatrix(*this);

#else
        fpgaBackend->postProcessAMatrix(*this);

#endif
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

    void AdapCholContext::setBackend(Backend *cpuBackend_, Backend *fpgaBackend_) {
        cpuBackend = cpuBackend_;
        fpgaBackend = fpgaBackend_;
    }


    double *AdapCholContext::getFrontal(int index) {
        return pF[index];
    }

}