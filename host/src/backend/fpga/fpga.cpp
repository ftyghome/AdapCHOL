#include "backend/fpga/xrt_wrapper.h"
#include "backend/fpga/fpga.h"
#include "utils.h"
#include "adapchol.h"
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <iostream>

#include "xrt/xrt_device.h"
#include "xrt/xrt_bo.h"
#include "xrt/xrt_kernel.h"

/* taskCtrl bit:
 * 0: whether the node is a leaf node
 * 1: whether the parentF should be cleared (processing the first child currently)
*/

namespace AdapChol {
    class AdapCholContext;

    bool FPGABackend::preComputeCU(AdapCholContext &context, int64_t col, int cuIdx) {
        auto &symbol = context.symbol;
        auto &pF = context.pF;
        auto &pFn = context.pFn;
        auto &L = context.L;
        unsigned char taskCtrl = 0;

        bool isLeaf = false;
        csi parent = symbol->parent[col];
        if (pF[col] == nullptr) {
            // if it's not a leaf, its descendant will allocate a frontal matrix for it.
            isLeaf = true;
            taskCtrl |= 0b1;
        }
        if (parent != -1) {
            csi parentFsize = (1 + pFn[parent]) * pFn[parent] / 2;
            if (pF[parent] == nullptr) {
                getFMemTimeCount += timedRun([&] {
                    const auto poolMem = getFMemFromPool(context);
                    pF[parent] = poolMem.first;
                    pF_buffer[parent] = poolMem.second;
                    taskCtrl |= 0b10;
                });
            }
            fillPTimeCount += timedRun([&] {
                context.fillP(P[cuIdx], col);
            });
        }
        rootNodeTimeCount += timedRun([&] {
            if (parent == -1) {
                if (L->p[col + 1] - L->p[col] > 0) {
                    double diag = sqrt(pF[col][0] + L->x[L->p[col]]);
                    for (size_t i = 0; i < L->p[col + 1] - L->p[col]; i++) {
                        L->x[L->p[col] + i] = i ? pF[col][i] / diag : diag;
                    }
                }
            }
        });
        if (parent == -1) return false;

        syncTimeCount += timedRun([&] {
            P_buffers[cuIdx]->sync(XCL_BO_SYNC_BO_TO_DEVICE, pFn[parent], 0);
        });
        auto &descF_buffer = pF_buffer[col], &parF_buffer = pF_buffer[parent];
        argSetTimeCount += timedRun([&] {
            if (isLeaf)
                runs[cuIdx]->set_arg(0, nullptr);
            else
                runs[cuIdx]->set_arg(0, *descF_buffer);
            runs[cuIdx]->set_arg(2, *parF_buffer);
            runs[cuIdx]->set_arg(4, (int) symbol->cp[col]);
            runs[cuIdx]->set_arg(5, (int) pFn[col]);
            runs[cuIdx]->set_arg(6, (int) pFn[parent]);
            runs[cuIdx]->set_arg(7, taskCtrl);
        });

        return true;
    }

    void FPGABackend::postComputeCU(AdapCholContext &context, int64_t col, int cuIdx) {
        auto &pF = context.pF;
        bool isLeaf = false;
        if (pF[col] == nullptr) {
            isLeaf = true;
        }
        returnFMemTimeCount += timedRun([&] {
            if (!isLeaf) {
                returnFMemToPool(context, pF[col], pF_buffer[col]);
                pF[col] = nullptr;
                pF_buffer[col] = nullptr;
            }
        });
    }

    void FPGABackend::processColumns(AdapCholContext &context, int *tasks, int length) {
//        std::cerr << "processing " << length << "as a whole\n";
        bool needCompute[length];
        for (int i = 0; i < length; i++) {
            needCompute[i] = preComputeCU(context, tasks[i], i);
        }
        kernelConstructRunTimeCount += timedRun([&] {
            for (int i = 0; i < length; i++) {
                if (needCompute[i])
                    runs[i]->start();
            }
        });
        waitTimeCount += timedRun([&] {
            for (int i = 0; i < length; i++) {
                if (needCompute[i])
                    while (runs[i]->state() != ERT_CMD_STATE_COMPLETED);
            }
        });
        for (int i = 0; i < length; i++) {
            if (needCompute[i])
                postComputeCU(context, tasks[i], i);
        }
    }

    void FPGABackend::processAColumn(AdapCholContext &context, int64_t col) {
        bool needCompute = preComputeCU(context, col, 0);
        if (needCompute) {
            runs[0]->start();
            while (runs[0]->state() != ERT_CMD_STATE_COMPLETED);
            postComputeCU(context, col, 0);
        }
    }

    FPGABackend::FPGABackend(const std::string &binaryFile, int cus_) :
            deviceContext(binaryFile, 0), cus(cus_) {
//        kernel = std::make_shared<xrt::kernel>();
        runs = new RunPtr[cus];
        for (int i = 0; i < cus_; i++) {
            runs[i] = std::make_shared<xrt::run>(
                    deviceContext.getKernel(std::string("krnl_proc_col:{krnl_proc_col_") + std::to_string(i) + "}"));
        }
    }

    std::pair<double *, BoPtr> FPGABackend::getFMemFromPool(AdapCholContext &context) {
        if (context.poolTail >= context.poolHead) {
            int idx = context.poolHead;
            Fpool[idx] =
                    std::make_shared<xrt::bo>(*deviceContext.getDevice(),
                                              sizeof(double) * (1 + context.maxFn) * context.maxFn / 2 + 16,
                                              XRT_BO_FLAGS_CACHEABLE,
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
        Lx_buffer->sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    }

    void FPGABackend::preProcessAMatrix(AdapCholContext &context) {
        preProcessAMatrixTimeCount += timedRun([&] {
            pF_buffer = new std::shared_ptr<xrt::bo>[context.n];
            Fpool = new std::shared_ptr<xrt::bo>[context.n];
            P_buffers = new BoPtr[cus];
            P = new bool *[cus];
            for (int i = 0; i < cus; i++) {
                P_buffers[i] = std::make_shared<xrt::bo>(*deviceContext.getDevice(),
                                                         sizeof(bool) * (context.maxFn + 32),
                                                         XRT_BO_FLAGS_HOST_ONLY,
                                                         FPGA_MEM_BANK_ID);
                P[i] = P_buffers[i]->map<bool *>();
            }
            for (int i = 0; i < cus; i++) {
                runs[i]->set_arg(1, *P_buffers[i]);
                runs[i]->set_arg(3, *Lx_buffer);
            }
            Lx_buffer->sync(XCL_BO_SYNC_BO_TO_DEVICE);
        });
    }

    bool *FPGABackend::allocateP(size_t bytes) {
        return nullptr;
    }

    int64_t FPGABackend::getTimeCount() {
        return waitTimeCount;
    }

    void FPGABackend::printStatistics() {
        std::cerr << "FPGA Backend Stat:"
                  << "\n\twaitTime: " << waitTimeCount
                  << "\n\tkernelConstrctTime: " << kernelConstructRunTimeCount
                  << "\n\tfillPTime: " << fillPTimeCount
                  << "\n\tgetFMemTime: " << getFMemTimeCount
                  << "\n\treturnFMemTime: " << returnFMemTimeCount
                  << "\n\tLeafCPU: " << LeafCPUTimeCount
                  << "\n\tSync: " << syncTimeCount
                  << "\n\tfirstColProc: " << firstColProcTimeCount
                  << "\n\tpreProcessAMatrix: " << preProcessAMatrixTimeCount
                  << "\n\trootNodeTime: " << rootNodeTimeCount
                  << "\n\targSetTime: " << argSetTimeCount
                  << std::endl;
    }

    void FPGABackend::allocateAndFillL(AdapCholContext &context) {
        auto &L = context.L;
        auto &n = context.n;
        auto symbol = context.symbol;
        auto AppL = context.AppL;
        auto App = context.App;

        Lx_buffer = std::make_shared<xrt::bo>(*deviceContext.getDevice(),
                                              sizeof(double) * symbol->cp[n],
                                              XRT_BO_FLAGS_NONE,
                                              FPGA_MEM_BANK_ID);

        L = adap_cs_spalloc_manual(n, n, symbol->cp[n], 0, Lx_buffer->map<double *>());
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
