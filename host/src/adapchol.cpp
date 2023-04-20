#include "adapchol.h"
#include "backend/cpu/cpu.h"
#include <cstring>
#include <cassert>

namespace AdapChol {

    void AdapCholContext::allocateAndFillL() {

        L = cs_spalloc(n, n, symbol->cp[n], 1, 0);
        memcpy(L->p, symbol->cp, sizeof(csi) * (n + 1));
        csi *tmpSW = (csi *) malloc(sizeof(csi) * 2 * n), *tmpS = tmpSW, *tmpW = tmpS + n;
        csi *LiPos = new csi[n + 1], *AiPos = new csi[n + 1];
        memcpy(LiPos, L->p, sizeof(csi) * (n + 1));
        memcpy(AiPos, AppL->p, sizeof(csi) * (n + 1));
        for (int col = 0; col < n; col++) {
            L->i[LiPos[col]] = col;
            while (AppL->i[AiPos[col]] < col && AiPos[col] < AppL->p[col + 1]) AiPos[col]++;
            if (AiPos[col] < AppL->p[col + 1] && AppL->i[AiPos[col]] == col) L->x[LiPos[col]] = AppL->x[AiPos[col]];
            LiPos[col]++;
        }
        memcpy(tmpW, symbol->cp, sizeof(csi) * n);
        for (int k = 0; k < n; k++) {
            csi top = cs_ereach(App, k, symbol->parent, tmpS, tmpW);
            for (csi i = top; i < n; i++) {
                csi col = tmpS[i];
                // L[index,k] is non-zero
                L->i[LiPos[col]] = k;
                while (AppL->i[AiPos[col]] < k && AiPos[col] < AppL->p[col + 1]) AiPos[col]++;
                if (AiPos[col] < AppL->p[col + 1] && AppL->i[AiPos[col]] == k) L->x[LiPos[col]] = AppL->x[AiPos[col]];
                LiPos[col]++;
            }
        }

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
        int indexP = 0;
        csi descRowIndexAlignWithPar = descRowBegin;
        for (csi parRowIndex = parRowBegin; parRowIndex < parRowEnd; parRowIndex++) {
            while (descRowIndexAlignWithPar < descRowEnd && L->i[descRowIndexAlignWithPar] < L->i[parRowIndex])
                descRowIndexAlignWithPar++;
            if (descRowIndexAlignWithPar < descRowEnd && L->i[descRowIndexAlignWithPar] == L->i[parRowIndex]) {
                // descendant has an element the same as the currently processing element in parent.
                // DESCENDANT  [1]     [4]
                // PARENT      [1] [2] [4]
                // when processing parent's [1] and [4], the descendant can be aligned, otherwise the processing column
                // of P is all zero.
                csi descRowIndex = descRowIndexAlignWithPar;
                for (csi row = parRowIndex; row < parRowEnd; row++) {
                    if (descRowIndex < descRowEnd && L->i[descRowIndex] == L->i[row]) {
                        P[indexP++] = true;
                        descRowIndex++;
                    } else {
                        P[indexP++] = false;
                    }
                }
            } else {
                for (csi row = parRowIndex; row < parRowEnd; row++) {
                    P[indexP++] = false;
                }
            }
        }

    }

    void AdapCholContext::prepareIndexingPointers() {
        pF = (double **) calloc(n, sizeof(double *));
        pFn = (csi *) malloc(sizeof(csi) * n);
        for (int i = 0; i < n; i++) {
            pFn[i] = symbol->cp[i + 1] - symbol->cp[i];
            if (pFn[i] > maxFn)
                maxFn = pFn[i];
        }
        publicP = (bool *) malloc(sizeof(bool) * (1 + maxFn) * maxFn / 2);
        Fpool = (double **) malloc(sizeof(double *) * n);
    }

    void AdapCholContext::run() {
        n = A->n;
        symbol = cs_schol(1, A);
        App = cs_symperm(A, symbol->pinv, 1);

        AppL = cs_transpose(App, 1);
        prepareIndexingPointers();
        allocateAndFillL();
        csi *post = cs_post(symbol->parent, n);
        for (int idx = 0; idx < n; idx++) {
            csi col = post[idx];
            cpuBackend->processAColumn(*this, col);
        }
    }

    double *AdapCholContext::getFMemFromPool() {
        if (poolTail >= poolHead)
            Fpool[poolHead++] = (double *) malloc(sizeof(double) * (1 + maxFn) * maxFn / 2);
        assert(poolTail < poolHead);
        return Fpool[poolTail++];
    }

    void AdapCholContext::returnFMemToPool(double *mem) {
        Fpool[--poolTail] = mem;
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

}