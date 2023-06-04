#include "backend/fpga/xrt_wrapper.h"
#include "backend/fpga/fpga.h"
#include "backend/cpu/cpu.h"
#include "internal/utils.h"
#include "internal/adapchol.h"
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <iostream>
#include <vector>

#include "xrt/xrt_device.h"
#include "xrt/xrt_bo.h"
#include "xrt/xrt_kernel.h"

/* taskCtrl bit:
 * 0: whether the node is a leaf node
 * 1: whether the parentF should be cleared (processing the first child currently)
*/

namespace AdapChol {
    class AdapCholContext;

    bool FPGABackend::preComputeCU(AdapCholContext &context, int col, int cuIdx, bool cpuFallback) {
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
            if (pF[parent] == nullptr) {
                TIMED_RUN_REGION_START(getFMemTimeCount)
                pF_Memory[parent] = getFMemFromPool(context, pFn[parent]);
                pF[parent] = pF_Memory[parent]->ptr;
                if (cpuFallback) {
                    memset(pF[parent], 0, sizeof(double) * (1 + pFn[parent]) * pFn[parent] / 2);
                    pF_Memory[parent]->doneCPUModify();
                } else {
                    pF_Memory[parent]->makeFPGAVisible();
                }
                taskCtrl |= 0b10;
                TIMED_RUN_REGION_END(getFMemTimeCount)
            }
            TIMED_RUN_REGION_START(fillPTimeCount)
            context.fillP(P[cuIdx], col);
            TIMED_RUN_REGION_END(fillPTimeCount)
        }
        TIMED_RUN_REGION_START(rootNodeTimeCount)
        if (parent == -1) {
            if (!isLeaf) {
                pF_Memory[col]->makeCPUVisible();
            }
            if (L->p[col + 1] - L->p[col] > 0) {
                double diag = sqrt(isLeaf ? L->x[L->p[col]] : pF[col][0] + L->x[L->p[col]]);
                for (size_t i = 0; i < L->p[col + 1] - L->p[col]; i++) {
                    L->x[L->p[col] + i] = i ? pF[col][i] / diag : diag;
                }
            }
        }
        TIMED_RUN_REGION_END(rootNodeTimeCount)
        if (parent == -1) return false;

        auto &descF_buffer = pF_Memory[col], &parF_buffer = pF_Memory[parent];

        if (cpuFallback) {
            if (!isLeaf)
                descF_buffer->makeCPUVisible();
            parF_buffer->makeCPUVisible();
        } else {
            if (!isLeaf)
                descF_buffer->makeFPGAVisible();
            parF_buffer->makeFPGAVisible();
            TIMED_RUN_REGION_START(syncTimeCount)
            P_buffers[cuIdx]->sync(XCL_BO_SYNC_BO_TO_DEVICE, pFn[parent], 0);
            TIMED_RUN_REGION_END(syncTimeCount)

            TIMED_RUN_REGION_START(argSetTimeCount)
            if (isLeaf)
                runs[cuIdx]->set_arg(0, nullptr);
            else
                runs[cuIdx]->set_arg(0, *(descF_buffer->content));
            runs[cuIdx]->set_arg(2, *(parF_buffer->content));
            runs[cuIdx]->set_arg(4, (int) symbol->cp[col]);
            runs[cuIdx]->set_arg(5, (int) pFn[col]);
            runs[cuIdx]->set_arg(6, (int) pFn[parent]);
            runs[cuIdx]->set_arg(7, taskCtrl);
            TIMED_RUN_REGION_END(argSetTimeCount)
        }
        return true;
    }

    void FPGABackend::postComputeCU(AdapCholContext &context, int col, int cuIdx, bool cpuFallback) {
        auto &pF = context.pF;
        bool isLeaf = false;
        if (pF[col] == nullptr) {
            isLeaf = true;
        }
        TIMED_RUN_REGION_START(returnFMemTimeCount)
        if (!isLeaf) {
            returnFMemToPool(context, pF_Memory[col], context.pFn[col]);
            pF[col] = nullptr;
            pF_Memory[col] = nullptr;
        }
        if (context.symbol->parent[col] != -1) {
            if(cpuFallback) pF_Memory[context.symbol->parent[col]]->doneCPUModify();
            else pF_Memory[context.symbol->parent[col]]->doneFPGAModify();
        }
        TIMED_RUN_REGION_END(returnFMemTimeCount)
    }

    void FPGABackend::processColumns(AdapCholContext &context, int *tasks, int length) {
        static bool needCompute[MAX_CU_SUPPORTED];
        static bool needCPUFallback[MAX_CU_SUPPORTED];
        std::sort(tasks, tasks + length, [&](const int &x, const int &y) {
//            return context.symbol->parent[x] == -1 ||
//                   (context.symbol->parent[y] != -1
//                    && context.pFn[context.symbol->parent[x]] > context.pFn[context.symbol->parent[y]]);
            return context.pFn[x] > context.pFn[y];
        });
        for (int i = 0; i < length; i++) {
            if (context.pFn[tasks[i]] > MAX_NZ_IN_A_COL ||
                (context.symbol->parent[tasks[i]] != -1 &&
                 context.pFn[context.symbol->parent[tasks[i]]] > MAX_NZ_IN_A_COL)) {
                needCPUFallback[i] = true;
            } else {
                needCPUFallback[i] = false;
            }
        }
        TIMED_RUN_REGION_START(kernelConstructRunTimeCount)
        for (int i = 0; i < length; i++) {
            if (!needCPUFallback[i]) {
                bool needComputeItem = preComputeCU(context, tasks[i], i, false);
                if (needComputeItem)
                    runs[i]->start();
                needCompute[i] = needComputeItem;
            }
        }
        TIMED_RUN_REGION_END(kernelConstructRunTimeCount)
        TIMED_RUN_REGION_START(waitTimeCount)
        for (int i = 0; i < length; i++) {
            if (!needCPUFallback[i]) {
                if (needCompute[i])
                    while (runs[i]->state() != ERT_CMD_STATE_COMPLETED);
                postComputeCU(context, tasks[i], i, false);
            }

        }
        TIMED_RUN_REGION_END(waitTimeCount)
        // Falling back to CPU
        for (int i = 0; i < length; i++) {
            if (needCPUFallback[i]) {
                bool needComputeItem = preComputeCU(context, tasks[i], 0, true);
                if (needComputeItem) {
                    cpuFBComputeCU(context, tasks[i]);
                }
                postComputeCU(context, tasks[i], 0, true);
            }
        }
    }

    void FPGABackend::processAColumn(AdapCholContext &context, int col) {
        bool needCompute = preComputeCU(context, col, 0, false);
        if (needCompute) {
            runs[0]->start();
            while (runs[0]->state() != ERT_CMD_STATE_COMPLETED);
            postComputeCU(context, col, 0, false);
        }
    }

    FPGABackend::FPGABackend(const std::string &binaryFile, int cus_) :
            deviceContext(binaryFile, 0), cus(cus_) {
        runs = new RunPtr[cus];
        for (int i = 0; i < cus_; i++) {
            runs[i] = new xrt::run(
                    deviceContext.getKernel(std::string("krnl_proc_col:{krnl_proc_col_") + std::to_string(i) + "}"));
        }
    }

    MemoryElement *FPGABackend::getFMemFromPool(AdapCholContext &context, csi Fn) {
        MemPool *memPool = Fn > context.poolSplitStd ? bigPool : tinyPool;
        return memPool->getMem();
    }

    void
    FPGABackend::returnFMemToPool(AdapCholContext &context, MemoryElement *mem_, csi Fn) {
        MemPool *memPool = Fn > context.poolSplitStd ? bigPool : tinyPool;
        memPool->returnMem(mem_);
    }

    void FPGABackend::postProcessAMatrix(AdapCholContext &context) {
        Lx_buffer->sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    }

    void FPGABackend::preProcessAMatrix(AdapCholContext &context) {
        TIMED_RUN_REGION_START(preProcessAMatrixTimeCount)
        tinyPool = new MemPool(&deviceContext, context.n,
                               (1 + context.poolSplitStd) * context.poolSplitStd / 2);
        bigPool = new MemPool(&deviceContext, context.n, (1 + context.maxFn) * context.maxFn / 2);
        P_buffers = new BoPtr[cus];
        pF_Memory = new MemoryElement *[context.n];
        P = new bool *[cus];
        for (int i = 0; i < cus; i++) {
            P_buffers[i] = new xrt::bo(*deviceContext.getDevice(),
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
        TIMED_RUN_REGION_END(preProcessAMatrixTimeCount)
    }

    bool *FPGABackend::allocateP(size_t bytes) {
        return nullptr;
    }

    int FPGABackend::getTimeCount() {
        return waitTimeCount;
    }

    void FPGABackend::printStatistics() {
        PERF_LOG("%s", "FPGA Backend Time Stat:")

        PERF_LOG("\twaitTime: %d", waitTimeCount)
        PERF_LOG("\tkernelConstrctTime: %d", kernelConstructRunTimeCount)
        PERF_LOG("\tfillPTime: %d", fillPTimeCount)
        PERF_LOG("\tgetFMemTime: %d", getFMemTimeCount)
        PERF_LOG("\treturnFMemTime: %d", returnFMemTimeCount)
        PERF_LOG("\tLeafCPU: %d", LeafCPUTimeCount)
        PERF_LOG("\tSync: %d", syncTimeCount)
        PERF_LOG("\tfirstColProc: %d", firstColProcTimeCount)
        PERF_LOG("\tpreProcessAMatrix: %d", preProcessAMatrixTimeCount)
        PERF_LOG("\trootNodeTime: %d", rootNodeTimeCount)
        PERF_LOG("\targSetTime: %d", argSetTimeCount)
        PERF_LOG("%s", "FPGA Backend Memory Stat:")
        PERF_LOG("\tbigPoolUsed: %d (length: %d)", bigPool->poolTop, bigPool->maxLength)
        PERF_LOG("\ttinyPoolUsed: %d (length: %d)", tinyPool->poolTop, tinyPool->maxLength)
    }

    void FPGABackend::allocateAndFillL(AdapCholContext &context) {
        auto &L = context.L;
        auto &n = context.n;
        auto symbol = context.symbol;
        auto AppL = context.AppL;
        auto App = context.App;

        Lx_buffer = new xrt::bo(*deviceContext.getDevice(),
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

    void FPGABackend::cpuFBComputeCU(AdapCholContext &context, int col) {
        bool isLeaf = false;
        csi parent = context.symbol->parent[col];
        auto L = context.L;
        auto pFn = context.pFn;
        auto pF = context.pF;
        if (context.pF[col] == nullptr) {
            isLeaf = true;
        }
        if (parent == -1) return;
        if (isLeaf) {
            CPUBackend::Sqrt_Div_Leaf(pFn[col], L->x + L->p[col]);
            CPUBackend::Gen_Update_Matrix_And_Write_Direct_Leaf(L->x + L->p[col], pF[parent], P[0],
                                                                pFn[col], pFn[parent]);
        } else {
            CPUBackend::Sqrt_Div(pF[col], pFn[col], L->x + L->p[col]);
            CPUBackend::Gen_Update_Matrix_And_Write_Direct(pF[col], pF[parent], P[0], pFn[col], pFn[parent]);
            CPUBackend::Result_Write(pF[col], L->x + L->p[col], pFn[col]);
        }
    }


    MemPool::MemPool(DeviceContext *deviceContext_, csi n, int maxLength_) {
        deviceContext = deviceContext_;
        maxLength = maxLength_;
        mems = new MemoryElement *[n];
    }

    MemoryElement *MemPool::getMem() {
        if (poolTop < 0) {
            poolTop++;
            mems[poolTop] = new MemoryElement;
            mems[poolTop]->content =
                    new xrt::bo(*deviceContext->getDevice(),
                                sizeof(double) * (maxLength + 32),
                                XRT_BO_FLAGS_CACHEABLE,
                                FPGA_MEM_BANK_ID
                    );
            mems[poolTop]->ptr = mems[poolTop]->content->map<double *>();
        }
        assert(poolTop >= 0);
        poolTop--;
        return mems[poolTop + 1];
    }

    void MemPool::returnMem(MemoryElement *mem_) {
        poolTop++;
        mems[poolTop] = mem_;
    }

    void MemoryElement::makeCPUVisible() {
        switch (status) {
            case MemoryElementStatus::FPGA_MODIFIED:
                content->sync(XCL_BO_SYNC_BO_FROM_DEVICE);
                status = MemoryElementStatus::COHERENT;
            default:
                return;
        }
    }

    void MemoryElement::makeFPGAVisible() {
        switch (status) {
            case MemoryElementStatus::CPU_WRITE_CACHED:
                content->sync(XCL_BO_SYNC_BO_TO_DEVICE);
                status = MemoryElementStatus::COHERENT;
            default:
                return;
        }
    }

    void MemoryElement::doneCPUModify() {
        status = MemoryElementStatus::CPU_WRITE_CACHED;
    }

    void MemoryElement::doneFPGAModify() {
        status = MemoryElementStatus::FPGA_MODIFIED;
    }
}
