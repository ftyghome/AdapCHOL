#pragma once

#include <cstdint>
#include <string>

namespace AdapChol {

    class Backend;

    class CPUBackend;

    class FPGABackend;

    class AdapCholContext;

    AdapCholContext *allocateContext();

    void setA(AdapCholContext *context, cs *A_);

    void setBackend(AdapCholContext *context, CPUBackend *cpuBackend_, FPGABackend *fpgaBackend_);

    void run(AdapCholContext *context);

    CPUBackend *allocateCPUBackend();

    FPGABackend *allocateFPGABackend(const std::string &binaryFile, int cus_);

    cs* getResult(AdapCholContext* context);

    cs *loadSparse(FILE *file);

    int getMemPoolUsage(AdapCholContext* context);

}


