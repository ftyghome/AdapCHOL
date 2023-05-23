#include "internal/adapchol.h"
#include "backend/cpu/cpu.h"
#include <cstring>
#include <cassert>
#include <iostream>

namespace AdapChol {

    void CPUBackend::processColumns(AdapCholContext &context, int *tasks, int length) {
        for (int i = 0; i < length; i++) {
            processAColumn(context, tasks[i]);
        }
    }

    void CPUBackend::processAColumn(AdapChol::AdapCholContext &context, csi col) {
        auto &symbol = context.symbol;
        auto &pF = context.pF;
        auto &pFn = context.pFn;
        auto &L = context.L;

        bool isLeaf = false;
        csi parent = symbol->parent[col];
        if (pF[col] == nullptr) {
            // if it's not a leaf, its descendant will allocate a frontal matrix for it.
            isLeaf = true;
        }
        if (parent != -1) {
            csi parentFsize = (1 + pFn[parent]) * pFn[parent] / 2;
            if (pF[parent] == nullptr) {
                pF[parent] = getFMemFromPool(context);
                memset(pF[parent], 0, sizeof(double) * parentFsize);
            }
            context.fillP(P, col);
        }
        if (isLeaf) {
            // it's a leaf, so we don't need a separate Frontal matrix, let's work in matrix L directly
            Sqrt_Div_Leaf(pFn[col], L->x + L->p[col]);
            if (parent != -1) {
                Gen_Update_Matrix_And_Write_Direct_Leaf(L->x + L->p[col], pF[parent], P,
                                                        pFn[col], pFn[parent]);
            }
        } else {
            Sqrt_Div(pF[col], pFn[col], L->x + L->p[col]);
            if (parent != -1)
                Gen_Update_Matrix_And_Write_Direct(pF[col], pF[parent], P, pFn[col], pFn[parent]);
            Result_Write(pF[col], L->x + L->p[col], pFn[col]);
            returnFMemToPool(context, pF[col]);
            pF[col] = nullptr;
        }

    }

    void CPUBackend::Sqrt_Div(double *F, int64_t Fn, double *L) {
        if (Fn <= 0) return;
        F[0] = sqrt(F[0] + L[0]);
        for (int i = 1; i < Fn; i++) {
            F[i] = (F[i] + L[i]) / F[0];
        }
    }

    void CPUBackend::Gen_Update_Matrix(const double *F, double *U, int64_t Fn) {
        int pos = 0;
        for (int i = 1; i < Fn; i++) {
            for (int j = i; j < Fn; j++) {
                U[pos] = F[Fn + pos] - F[i] * F[j];
                pos++;
            }
        }
    }

    void CPUBackend::Gen_Update_Matrix_Leaf(const double *F, double *U, int64_t Fn) {
        int pos = 0;
        for (int i = 1; i < Fn; i++) {
            for (int j = i; j < Fn; j++) {
                U[pos] = -F[i] * F[j];
                pos++;
            }
        }
    }

    void CPUBackend::Extern_Add(double *dest_F, const double *U, const bool *P, int64_t Fn) {
        int64_t Fsize = (1 + Fn) * Fn / 2;
        int uPos = 0;
        for (int i = 0; i < Fsize; i++) {
            if (P[i]) {
                dest_F[i] += U[uPos];
                uPos++;
            }
        }
    }

    void CPUBackend::Result_Write(const double *F, double *Cx, int64_t Fn) {
        for (int i = 0; i < Fn; i++) {
            Cx[i] = F[i];
        }
    }

    void CPUBackend::Sqrt_Div_Leaf(int64_t Fn, double *Lx) {
        if (Fn <= 0) return;
        Lx[0] = sqrt(Lx[0]);
        for (int i = 1; i < Fn; i++) {
            Lx[i] = Lx[i] / Lx[0];
        }
    }

    void
    CPUBackend::Gen_Update_Matrix_And_Write_Direct(const double *descF, double *parF, const bool *P, int64_t descFn,
                                                   int64_t parFn) {
        int UpdPos = 0, FPos = 0, PMajorIdx = 0, PMinorIdx = 0;
        double currentVal;
        for (int i = 1; i < descFn; i++) {
            for (int j = i; j < descFn; j++) {
                currentVal = descF[descFn + UpdPos] - descF[i] * descF[j];
                UpdPos++;
                while (!(P[PMajorIdx] && P[PMinorIdx])) {
                    if (!P[PMajorIdx]) {
                        FPos += (int) parFn - PMajorIdx;
                        PMinorIdx = ++PMajorIdx;
                        continue;
                    }
                    FPos++;
                    if (PMinorIdx == parFn - 1) {
                        PMinorIdx = ++PMajorIdx;
                    } else {
                        PMinorIdx++;
                    }
                }
                parF[FPos++] += currentVal;
                if (PMinorIdx == parFn - 1) {
                    PMajorIdx++;
                    PMinorIdx = PMajorIdx;
                } else {
                    PMinorIdx++;
                }
            }
        }
    }

    void CPUBackend::Gen_Update_Matrix_And_Write_Direct_Leaf(const double *descF, double *parF, const bool *P,
                                                             int64_t descFn, int64_t parFn) {
        int FPos = 0, PMajorIdx = 0, PMinorIdx = 0;
        double currentVal;
        for (int i = 1; i < descFn; i++) {
            for (int j = i; j < descFn; j++) {
                currentVal = -descF[i] * descF[j];
                while (!(P[PMajorIdx] && P[PMinorIdx])) {
                    if (!P[PMajorIdx]) {
                        FPos += (int) parFn - PMajorIdx;
                        PMinorIdx = ++PMajorIdx;
                        continue;
                    }
                    FPos++;
                    if (PMinorIdx == parFn - 1) {
                        PMinorIdx = ++PMajorIdx;
                    } else {
                        PMinorIdx++;
                    }
                }
                parF[FPos++] += currentVal;
                if (PMinorIdx == parFn - 1) {
                    PMajorIdx++;
                    PMinorIdx = PMajorIdx;
                } else {
                    PMinorIdx++;
                }
            }
        }
    }

    double *CPUBackend::getFMemFromPool(AdapCholContext &context) {
        if (context.poolTail >= context.poolHead)
            context.Fpool[context.poolHead++] = (double *) malloc(
                    sizeof(double) * (1 + context.maxFn) * context.maxFn / 2);
        assert(context.poolTail < context.poolHead);
        return context.Fpool[context.poolTail++];
    }

    void CPUBackend::returnFMemToPool(AdapCholContext &context, double *mem) {
        context.Fpool[--context.poolTail] = mem;
    }

    bool *CPUBackend::allocateP(size_t bytes) {
        return (bool *) malloc(bytes);
    }

    void CPUBackend::preProcessAMatrix(AdapCholContext &context) {
        P = (bool *) malloc(sizeof(bool) * context.maxFn + 64);
    }

    void CPUBackend::postProcessAMatrix(AdapCholContext &context) {

    }

    int64_t CPUBackend::getTimeCount() {
        return 0;
    }

    void CPUBackend::printStatistics() {
        std::cerr << "CPU Backend Stat: nothing to print." << '\n';
    }

    void CPUBackend::allocateAndFillL(AdapCholContext &context) {
        auto &L = context.L;
        auto &n = context.n;
        auto symbol = context.symbol;
        auto AppL = context.AppL;
        auto App = context.App;

        L = cs_spalloc(n, n, symbol->cp[n], 1, 0);
        memcpy(L->p, symbol->cp, sizeof(csi) * (n + 1));
        csi *tmpSW = (csi *) malloc(sizeof(csi) * 2 * n), *tmpS = tmpSW, *tmpW = tmpS + n;
        csi *LiPos = new csi[n + 1], *AiPos = new csi[n + 1];
        memcpy(LiPos, L->p, sizeof(csi) * (n + 1));
        memcpy(AiPos, AppL->p, sizeof(csi) * (n + 1));

        // skip upper triangle

        for (int col = 0; col < n; col++) {
            L->i[LiPos[col]] = col;
            while (AppL->i[AiPos[col]] < col && AiPos[col] < AppL->p[col + 1]) AiPos[col]++;
            if (AiPos[col] < AppL->p[col + 1] && AppL->i[AiPos[col]] == col) L->x[LiPos[col]] = AppL->x[AiPos[col]];
            LiPos[col]++;
        }

        memcpy(tmpW, symbol->cp, sizeof(csi) * n);
        for (int k = 0; k < n; k++) {
            csi top;
            top = cs_ereach(App, k, symbol->parent, tmpS, tmpW);
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
    }


}

