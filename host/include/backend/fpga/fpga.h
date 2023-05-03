#pragma once

#define MAX_CU_SUPPORTED 8

extern "C" {
#include "csparse/Include/cs.h"
}

#include "backend/common.h"
#include "xrt_wrapper.h"
#include <cstdint>
#include <cmath>

namespace AdapChol {
    class AdapCholContext;

    struct MemPool {
        BoPtr *content = nullptr;
        double **content_ptr = nullptr;
        int poolTop = -1;
        int maxLength;
        DeviceContext *deviceContext = nullptr;

        MemPool(DeviceContext *deviceContext, csi n, int maxLength_);

        std::pair<double *, BoPtr> getMem();

        void returnMem(double *ptr, BoPtr buffer);
    };

    class FPGABackend : public Backend {
    private:
        DeviceContext deviceContext;
        KernelPtr kernel;
        BoPtr *P_buffers;
        BoPtr *pF_buffer;
        MemPool *tinyPool, *bigPool;
        BoPtr Lx_buffer;
        RunPtr *runs;
        bool **P;
        int cus = 1;
        int64_t waitTimeCount = 0, fillPTimeCount = 0, LeafCPUTimeCount = 0,
                getFMemTimeCount = 0, syncTimeCount = 0, firstColProcTimeCount = 0,
                preProcessAMatrixTimeCount = 0, returnFMemTimeCount = 0, kernelConstructRunTimeCount = 0,
                rootNodeTimeCount = 0, argSetTimeCount = 0;

        bool preComputeCU(AdapCholContext &context, csi col, int cuIdx);

        void postComputeCU(AdapCholContext &context, csi col, int cuIdx);

    public:
        FPGABackend(const std::string &binaryFile, int cus_);

        void preProcessAMatrix(AdapChol::AdapCholContext &context) override;

        void postProcessAMatrix(AdapChol::AdapCholContext &context) override;

        void processColumns(AdapChol::AdapCholContext &context, int *tasks, int length);

        void processAColumn(AdapChol::AdapCholContext &context, csi col);

        std::pair<double *, BoPtr> getFMemFromPool(AdapChol::AdapCholContext &context, csi Fn);

        void returnFMemToPool(AdapChol::AdapCholContext &context, double *ptr, BoPtr buffer, csi Fn);

        bool *allocateP(size_t bytes) override;

        int64_t getTimeCount() override;

        void printStatistics() override;

        void allocateAndFillL(AdapChol::AdapCholContext &context) override;


    };
}
