#pragma once

extern "C" {
#include "csparse/Include/cs.h"
}

#include "backend/common.h"
#include "xrt_wrapper.h"
#include <cstdint>
#include <cmath>

namespace AdapChol {
    class AdapCholContext;

    class FPGABackend : public Backend {
    private:
        DeviceContext deviceContext;
        KernelPtr kernel;
        BoPtr P_buffer;
        BoPtr *pF_buffer;
        BoPtr *Fpool;
        BoPtr Lx_buffer;
        RunPtr run;
        int64_t waitTimeCount = 0, fillPTimeCount = 0, LeafCPUTimeCount = 0,
                getFMemTimeCount = 0, syncTimeCount = 0, firstColProcTimeCount = 0,
                preProcessAMatrixTimeCount = 0, returnFMemTimeCount = 0, kernelConstructRunTimeCount = 0;
    public:
        FPGABackend(const std::string &binaryFile);

        void preProcessAMatrix(AdapChol::AdapCholContext &context) override;

        void postProcessAMatrix(AdapChol::AdapCholContext &context) override;

        void processAColumn(AdapChol::AdapCholContext &context, csi col);

        std::pair<double *, BoPtr> getFMemFromPool(AdapChol::AdapCholContext &context);

        void returnFMemToPool(AdapChol::AdapCholContext &context, double *mem, BoPtr mem_buffer);

        bool *allocateP(size_t bytes) override;

        int64_t getTimeCount() override;

        void printStatistics() override;

        void allocateAndFillL(AdapChol::AdapCholContext &context) override;


    };
}
