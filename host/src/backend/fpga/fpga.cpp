#include "backend/fpga/xrt_wrapper.h"
#include "backend/fpga/fpga.h"
#include "adapchol.h"
#include <cstring>

#include "xrt/xrt_device.h"
#include "xrt/xrt_bo.h"
#include "xrt/xrt_kernel.h"

namespace AdapChol {
    class AdapCholContext;

    void FPGABackend::processAColumn(AdapCholContext &context, int64_t col) {
        auto &symbol = context.symbol;
        auto &pF = context.pF;
        auto &pFn = context.pFn;
        auto &L = context.L;
        auto &publicP = context.publicP;

        bool isLeaf = false;
        csi parent = symbol->parent[col];
        if (pF[col] == nullptr) {
            // if it's not a leaf, its descendant will allocate a frontal matrix for it.
            isLeaf = true;
        }
        if (parent != -1) {
            csi parentFsize = (1 + pFn[parent]) * pFn[parent] / 2;
            if (pF[parent] == nullptr) {
                pF[parent] = context.getFMemFromPool();
                memset(pF[parent], 0, sizeof(double) * parentFsize);
            }
            context.fillP(col);
        }
        if (isLeaf) {
            // it's a leaf, so we don't need a separate Frontal matrix, let's work in matrix L directly
            Sqrt_Div_Leaf(pFn[col], L->x + L->p[col]);
            if (parent != -1) {
                Gen_Update_Matrix_And_Write_Direct_Leaf(L->x + L->p[col], pF[parent], publicP,
                                                        pFn[col]);
            }
        } else {
            Sqrt_Div(pF[col], pFn[col], L->x + L->p[col]);
            if (parent != -1)
                Gen_Update_Matrix_And_Write_Direct(pF[col], pF[parent], publicP, pFn[col]);
            Result_Write(pF[col], L->x + L->p[col], pFn[col]);
            context.returnFMemToPool(pF[col]);
            pF[col] = nullptr;
        }
    }

    FPGABackend::FPGABackend(const std::string &binaryFile) :
            deviceContext(binaryFile, 1) {
        kernel = std::make_shared<xrt::kernel>(deviceContext.getKernel("krnl_proc_col"));
    }

    void FPGABackend::Sqrt_Div(double *F, int64_t Fn, double *L) {
        if (Fn <= 0) return;
        F[0] = sqrt(F[0] + L[0]);
        for (int i = 1; i < Fn; i++) {
            F[i] = (F[i] + L[i]) / F[0];
        }
    }

    void FPGABackend::Gen_Update_Matrix(const double *F, double *U, int64_t Fn) {
        int pos = 0;
        for (int i = 1; i < Fn; i++) {
            for (int j = i; j < Fn; j++) {
                U[pos] = F[Fn + pos] - F[i] * F[j];
                pos++;
            }
        }
    }

    void FPGABackend::Gen_Update_Matrix_Leaf(const double *F, double *U, int64_t Fn) {
        int pos = 0;
        for (int i = 1; i < Fn; i++) {
            for (int j = i; j < Fn; j++) {
                U[pos] = -F[i] * F[j];
                pos++;
            }
        }
    }

    void FPGABackend::Extern_Add(double *dest_F, const double *U, const bool *P, int64_t Fn) {
        int64_t Fsize = (1 + Fn) * Fn / 2;
        int uPos = 0;
        for (int i = 0; i < Fsize; i++) {
            if (P[i]) {
                dest_F[i] += U[uPos];
                uPos++;
            }
        }
    }

    void FPGABackend::Result_Write(const double *F, double *Cx, int64_t Fn) {
        for (int i = 0; i < Fn; i++) {
            Cx[i] = F[i];
        }
    }

    void FPGABackend::Sqrt_Div_Leaf(int64_t Fn, double *Lx) {
        if (Fn <= 0) return;
        Lx[0] = sqrt(Lx[0]);
        for (int i = 1; i < Fn; i++) {
            Lx[i] = Lx[i] / Lx[0];
        }
    }

    void
    FPGABackend::Gen_Update_Matrix_And_Write_Direct(const double *descF, double *parF, const bool *P, int64_t descFn) {
        int UpdPos = 0, FPos = 0;
        double currentVal;
        for (int i = 1; i < descFn; i++) {
            for (int j = i; j < descFn; j++) {
                currentVal = descF[descFn + UpdPos] - descF[i] * descF[j];
                UpdPos++;
                while (P[FPos] == 0) FPos++;
                parF[FPos] += currentVal;
                FPos++;
            }
        }
    }

    void FPGABackend::Gen_Update_Matrix_And_Write_Direct_Leaf(const double *descF, double *parF, const bool *P,
                                                              int64_t descFn) {
        int UpdPos = 0, FPos = 0;
        double currentVal;
        for (int i = 1; i < descFn; i++) {
            for (int j = i; j < descFn; j++) {
                currentVal = -descF[i] * descF[j];
                UpdPos++;
                while (P[FPos] == 0) FPos++;
                parF[FPos] += currentVal;
                FPos++;
            }
        }
    }
}
