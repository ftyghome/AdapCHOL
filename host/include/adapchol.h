#pragma once

#include "backend/common.h"

extern "C" {
#include "csparse/Include/cs.h"
}

namespace AdapChol {

    class CPUBackend;
    class FPGABackend;

    class AdapCholContext {
        csi n = 0;
        double **pF = nullptr;
        bool *publicP = nullptr;
        cs *A = nullptr, *App = nullptr, *AppL = nullptr;
        csi *pFn = nullptr;
        css *symbol = nullptr;
        cs *L = nullptr;
        double **Fpool = nullptr;
        int poolHead = 0, poolTail = 0;
        csi maxFn = 0;

        Backend *cpuBackend = nullptr, *fpgaBackend = nullptr;

        friend class AdapChol::CPUBackend;
        friend class AdapChol::FPGABackend;

    public:
        AdapCholContext() = default;

        [[nodiscard]] int getMemPoolUsage() const;

        void run();

        cs *getResult();

        void setA(cs *A_);

        void setBackend(Backend *cpuBackend_, Backend *fpgaBackend_);

        double* getFrontal(int index);

    protected:
        void prepareIndexingPointers();

        void allocateAndFillL();

        void fillP(csi col);

        void *mallocAligned(size_t bytes);

    };
}


