#pragma once

#include "backend/cpu.h"

extern "C" {
#include "csparse/Include/cs.h"
}

namespace AdapChol {
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

        friend void AdapChol::CPUBackend::processAColumn(AdapCholContext &, csi);

    public:
        explicit AdapCholContext(cs *A);

        int getMemPoolUsage() const;

        void run();

        cs *getResult();

    protected:
        void prepareIndexingPointers();

        void allocateAndFillL();

        void fillP(csi col);

        double *getFMemFromPool();

        void returnFMemToPool(double *mem);


    };
}


