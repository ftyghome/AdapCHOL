#include "backend/fpga/xrt_wrapper.h"
#include "backend/fpga/fpga.h"
#include "utils.h"
#include "adapchol.h"
#include <cstring>
#include <cassert>
#include <iostream>

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
                const auto poolMem = getFMemFromPool(context);
                pF[parent] = poolMem.first;
                pF_buffer[parent] = poolMem.second;
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
            if (L->p[col + 1] - L->p[col] > 0) {
                double diag = sqrt(pF[col][0] + L->x[L->p[col]]);
                for (size_t i = 0; i < L->p[col + 1] - L->p[col]; i++) {
                    pF[col][i] += L->x[L->p[col] + i];
                    L->x[L->p[col] + i] = i ? pF[col][i] / diag : diag;
                }
            }
            if (parent == -1) return;
            P_buffer->sync(XCL_BO_SYNC_BO_TO_DEVICE);
            auto &descF_buffer = pF_buffer[col], &parF_buffer = pF_buffer[parent];
            descF_buffer->sync(XCL_BO_SYNC_BO_TO_DEVICE);
            parF_buffer->sync(XCL_BO_SYNC_BO_TO_DEVICE);
            auto run = (*kernel)(*descF_buffer, *P_buffer, *parF_buffer, (int) pFn[col], (int) pFn[parent]);
            int64_t runTime = timedRun([&] {
                run.wait();
            });
            timeCount += runTime;
            parF_buffer->sync(XCL_BO_SYNC_BO_FROM_DEVICE);
            returnFMemToPool(context, pF[col], pF_buffer[col]);
            pF[col] = nullptr;
            pF_buffer[col] = nullptr;
        }
    }

    FPGABackend::FPGABackend(const std::string &binaryFile) :
            deviceContext(binaryFile, 0) {
        kernel = std::make_shared<xrt::kernel>(deviceContext.getKernel("krnl_proc_col"));
    }


    void FPGABackend::Sqrt_Div_Leaf(int64_t Fn, double *Lx) {
        if (Fn <= 0) return;
        Lx[0] = sqrt(Lx[0]);
        for (int i = 1; i < Fn; i++) {
            Lx[i] = Lx[i] / Lx[0];
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

    std::pair<double *, BoPtr> FPGABackend::getFMemFromPool(AdapCholContext &context) {
        if (context.poolTail >= context.poolHead) {
            int idx = context.poolHead;
            Fpool[idx] =
                    std::make_shared<xrt::bo>(*deviceContext.getDevice(),
                                              sizeof(double) * (1 + context.maxFn) * context.maxFn / 2 + 16,
                                              XRT_BO_FLAGS_HOST_ONLY,
                                              FPGA_MEM_BANK_ID
                    );
            context.Fpool[idx] = Fpool[idx]->map<double *>();
            context.poolHead++;
        }
        assert(context.poolTail < context.poolHead);
        context.poolTail++;
        return {context.Fpool[context.poolTail - 1], Fpool[context.poolTail - 1]};
    }

    void FPGABackend::returnFMemToPool(AdapCholContext &context, double *mem, BoPtr mem_buffer) {
        int idx = --context.poolTail;
        Fpool[idx] = std::move(mem_buffer);
        context.Fpool[idx] = mem;
    }

    void FPGABackend::postProcessAMatrix(AdapCholContext &context) {

    }

    void FPGABackend::preProcessAMatrix(AdapCholContext &context) {
        pF_buffer = new std::shared_ptr<xrt::bo>[context.n];
        Fpool = new std::shared_ptr<xrt::bo>[context.n];
        P_buffer = std::make_shared<xrt::bo>(*deviceContext.getDevice(),
                                             sizeof(bool) * (1 + context.maxFn) * context.maxFn / 2 + 16,
                                             XRT_BO_FLAGS_HOST_ONLY,
                                             FPGA_MEM_BANK_ID);
        context.publicP = P_buffer->map<bool *>();
    }

    bool *FPGABackend::allocateP(size_t bytes) {
        return nullptr;
    }

    int64_t FPGABackend::getTimeCount() {
        return timeCount;
    }

}
