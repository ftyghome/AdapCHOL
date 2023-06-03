#include <fstream>
#include <cassert>
#include <chrono>
#include <functional>
#include "adapchol.h"
#include "internal/utils.h"

int main(int args, char *argv[]) {
    std::ofstream resultStream(argv[1]);
    std::ofstream csparseResultStream(argv[2]);
    std::ofstream dispatchPerfStream(argv[3]);
    int adapcholTime = 0, csparseTime = 0;
    cs *A = AdapChol::loadSparse(stdin);

    auto *cpuBackend = AdapChol::allocateCPUBackend();
    auto *fpgaBackend = AdapChol::allocateFPGABackend(
            std::string("/lib/firmware/xilinx/adapchol/binary_container_1.bin"), 4);
    AdapChol::AdapCholContext *m_context = AdapChol::allocateContext();
    AdapChol::setA(m_context, A);
    AdapChol::setBackend(m_context, cpuBackend, fpgaBackend);
    TIMED_RUN_REGION_START_ALWAYS(adapcholTime)
    AdapChol::run(m_context);
    TIMED_RUN_REGION_END_ALWAYS(adapcholTime)
    auto *result = AdapChol::getResult(m_context);
    AdapChol::dumpFormalResult(resultStream, result);
    csn *csparse_result;
    TIMED_RUN_REGION_START_ALWAYS(csparseTime)
    css *symbol = AdapChol::cs_schol(1, (cs *) A);
    csparse_result = AdapChol::cs_chol((cs *) A, symbol);
    TIMED_RUN_REGION_END_ALWAYS(csparseTime)
    AdapChol::dumpFormalResult(csparseResultStream, AdapChol::getL(csparse_result));
    std::vector<int> dispatchPerf = AdapChol::getDispatchPerfData(m_context);
    int dispatchCount[5] = {0};
    int currentVal = -1, currentCount = 0;
    for (const auto &i: dispatchPerf) {
        dispatchCount[i]++;
    }
    dispatchPerfStream << "{\n";
    for (int i = 1; i <= 4; i++) {
        dispatchPerfStream << "\t\"" << i << "\": " << dispatchCount[i] << (i == 4 ? "\n" : ",\n");
    }
    dispatchPerfStream << "}\n";
    printf("adapchol time: %d, csparse time: %d, speed %.2fx\n", adapcholTime, csparseTime,
           (double) csparseTime / (double) adapcholTime);
    printf("adapchol mem pool used: %d\n", AdapChol::getMemPoolUsage(m_context));
}
