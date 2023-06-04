#pragma once

#define MAX_CU_SUPPORTED 8
#define MAX_NZ_IN_A_COL 600

extern "C" {
#include "csparse/Include/cs.h"
}

#include "backend/common.h"
#include "xrt_wrapper.h"
#include <cstdint>
#include <cmath>

namespace AdapChol {
    class AdapCholContext;

    enum class MemoryElementStatus {
        COHERENT, CPU_WRITE_CACHED, FPGA_MODIFIED
    };

    struct MemoryElement {
        /* we need to know whether a memory is modified from CPU and getting cached, or the flushing of CPU cache
         * may just destroy the modification on FPGA side
         */
        MemoryElementStatus status = MemoryElementStatus::COHERENT;
        BoPtr content = nullptr;
        double *ptr = nullptr;

        void makeCPUVisible();

        void makeFPGAVisible();

        void doneCPUModify();

        void doneFPGAModify();

    };

    struct MemPool {
        MemoryElement **mems = nullptr;
        int poolTop = -1;
        int maxLength;
        DeviceContext *deviceContext = nullptr;

        MemPool(DeviceContext *deviceContext, csi n, int maxLength_);

        MemoryElement *getMem();

        void returnMem(MemoryElement *mem_);
    };

    class FPGABackend : public Backend {
    private:
        DeviceContext deviceContext;
        KernelPtr kernel;
        BoPtr *P_buffers;
        MemoryElement **pF_Memory;
        MemPool *tinyPool, *bigPool;
        BoPtr Lx_buffer;
        RunPtr *runs;
        bool **P;
        int cus = 1;
        int waitTimeCount = 0, fillPTimeCount = 0, LeafCPUTimeCount = 0,
                getFMemTimeCount = 0, syncTimeCount = 0, firstColProcTimeCount = 0,
                preProcessAMatrixTimeCount = 0, returnFMemTimeCount = 0, kernelConstructRunTimeCount = 0,
                rootNodeTimeCount = 0, argSetTimeCount = 0;

        bool preComputeCU(AdapCholContext &context, int col, int cuIdx, bool cpuFallback = false);

        void cpuFBComputeCU(AdapCholContext &context, csi col);

        void postComputeCU(AdapCholContext &context, int col, int cuIdx, bool cpuFallback);

    public:
        FPGABackend(const std::string &binaryFile, int cus_);

        void preProcessAMatrix(AdapChol::AdapCholContext &context) override;

        void postProcessAMatrix(AdapChol::AdapCholContext &context) override;

        void processColumns(AdapChol::AdapCholContext &context, int *tasks, int length);

        void processAColumn(AdapChol::AdapCholContext &context, csi col);

        MemoryElement *getFMemFromPool(AdapChol::AdapCholContext &context, csi Fn);

        void
        returnFMemToPool(AdapCholContext &context, MemoryElement *mem_, csi Fn);

        bool *allocateP(size_t bytes) override;

        int getTimeCount() override;

        void printStatistics() override;

        void allocateAndFillL(AdapChol::AdapCholContext &context) override;


    };
}
