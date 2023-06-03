#pragma once

#include "common.h"

#include <string>

#include <vector>

#include "backend/common.h"

#include "utils.h"

#include "internal/cs_adap/cs_adap.h"

namespace AdapChol {

    class CPUBackend;

    class FPGABackend;

    class AdapCholContext {
        csi n = 0;
        double **pF = nullptr;
        cs *A = nullptr, *App = nullptr, *AppL = nullptr;
        csi *pFn = nullptr;
        adap_css *symbol = nullptr;
        cs *L = nullptr;
        double **Fpool = nullptr;
        int poolHead = 0, poolTail = 0;
        csi maxFn = 0, poolSplitStd = 0;

        std::vector<int> dispatchPerf;

        Backend *cpuBackend = nullptr, *fpgaBackend = nullptr;

        friend class AdapChol::CPUBackend;

        friend class AdapChol::FPGABackend;

    public:

        [[nodiscard]] int getMemPoolUsage() const;

        void run();

        cs *getResult();

        void setA(cs *A_);

        void setBackend(CPUBackend *cpuBackend_, FPGABackend *fpgaBackend_);

        double *getFrontal(int index);

        void postSolve(double *b);

        std::vector<int> getDispatchPerfData();

    protected:
        void prepareIndexingPointers();

        void fillP(bool *P, csi col);

    };

    void setA(AdapCholContext *context, cs *A_);

    CPUBackend *allocateCPUBackend();

    FPGABackend *allocateFPGABackend(const std::string &binaryFile, int cus_);

    void setBackend(AdapCholContext *context, CPUBackend *cpuBackend_, FPGABackend *fpgaBackend_);

    void run(AdapCholContext *context);

    cs *loadSparse(FILE *file);

    cs *getResult(AdapCholContext *context);

    int getMemPoolUsage(AdapCholContext *context);

    std::vector<int> getDispatchPerfData(AdapCholContext *context);

}


