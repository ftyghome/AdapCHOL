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
        int64_t ereachTime = 0, firstLoopTime = 0, secondLoopTime = 0;
        L = cs_spalloc(n, n, symbol->cp[n], 1, 0);
        memcpy(L->p, symbol->cp, sizeof(csi) * (n + 1));
        csi *tmpSW = (csi *) malloc(sizeof(csi) * 2 * n), *tmpS = tmpSW, *tmpW = tmpS + n;
        csi *LiPos = new csi[n + 1], *AiPos = new csi[n + 1];
        memcpy(LiPos, L->p, sizeof(csi) * (n + 1));
        memcpy(AiPos, AppL->p, sizeof(csi) * (n + 1));

        // skip upper triangle

        firstLoopTime = timedRun([&] {
            for (int col = 0; col < n; col++) {
                L->i[LiPos[col]] = col;
                while (AppL->i[AiPos[col]] < col && AiPos[col] < AppL->p[col + 1]) AiPos[col]++;
                if (AiPos[col] < AppL->p[col + 1] && AppL->i[AiPos[col]] == col) L->x[LiPos[col]] = AppL->x[AiPos[col]];
                LiPos[col]++;
            }
        });

        memcpy(tmpW, symbol->cp, sizeof(csi) * n);
        secondLoopTime = timedRun([&] {
            for (int k = 0; k < n; k++) {
                csi top;
                ereachTime += timedRun([&] {
                    top = cs_ereach(App, k, symbol->parent, tmpS, tmpW);
                });
                for (csi i = top; i < n; i++) {
                    csi col = tmpS[i];
                    // L[index,k] is non-zero
                    L->i[LiPos[col]] = k;
                    while (AppL->i[AiPos[col]] < k && AiPos[col] < AppL->p[col + 1]) AiPos[col]++;
                    if (AiPos[col] < AppL->p[col + 1] && AppL->i[AiPos[col]] == k)
                        L->x[LiPos[col]] = AppL->x[AiPos[col]];
                    LiPos[col]++;
                }
            }
        });
        std::cout << "firstLoopTime: " << firstLoopTime << " " << "secondLoopTime: " << secondLoopTime << '\n';
        std::cout << "ereachTime: " << ereachTime << '\n';
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
//        csi descRowIndexAlignWithPar = descRowBegin;
//        for (csi parRowIndex = parRowBegin; parRowIndex < parRowEnd; parRowIndex++) {
//            while (descRowIndexAlignWithPar < descRowEnd && L->i[descRowIndexAlignWithPar] < L->i[parRowIndex])
//                descRowIndexAlignWithPar++;
//            if (descRowIndexAlignWithPar < descRowEnd && L->i[descRowIndexAlignWithPar] == L->i[parRowIndex]) {
//                // descendant has an element the same as the currently processing element in parent.
//                // DESCENDANT  [1]     [4]
//                // PARENT      [1] [2] [4]
//                // when processing parent's [1] and [4], the descendant can be aligned, otherwise the processing column
//                // of P is all zero.
//                csi descRowIndex = descRowIndexAlignWithPar;
//                for (csi row = parRowIndex; row < parRowEnd; row++) {
//                    if (descRowIndex < descRowEnd && L->i[descRowIndex] == L->i[row]) {
//                        P[indexP++] = true;
//                        descRowIndex++;
//                    } else {
//                        P[indexP++] = false;
//                    }
//                }
//            } else {
//                for (csi row = parRowIndex; row < parRowEnd; row++) {
//                    P[indexP++] = false;
//                }
//            }
//        }

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
            prepTime = timedRun([&] {
                prepareIndexingPointers();
            });
            LRelatedTime = timedRun([&] {
                allocateAndFillL();
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
        if (fpgaBackend)
            fpgaBackend->printStatistics();
//        std::vector<int> test_cols = {0, 1};
//        for (auto &col: test_cols) {
//#if defined(__x86_64__) || defined(_M_X64)
//            cpuBackend->processAColumn(*this, col);
//#else
//            fpgaBackend->processAColumn(*this, col);
//#endif
//        }
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