#include <fstream>
#include <cassert>
#include <chrono>
#include <iostream>
#include <functional>
#include "adapchol.h"
#include "internal/utils.h"

int main(int args, char *argv[]) {
    std::ofstream resultStream(argv[1]);
    std::ofstream csparseResultStream(argv[2]);
    std::ofstream frontalStream(argv[3]), updateStream(argv[4]), pStream(argv[5]);
    int adapcholTime = 0, csparseTime = 0;
    auto *A = AdapChol::loadSparse(stdin);
    int n = AdapChol::getOrder(A);
    auto *b1 = new double[n], *b2 = new double[n];
    for (int i = 0; i < n; i++) b1[i] = b2[i] = i + 1;

    auto *cpuBackend = AdapChol::allocateCPUBackend();
    auto *fpgaBackend = AdapChol::allocateFPGABackend(
            std::string("/lib/firmware/xilinx/adapchol/binary_container_1.bin"), 4);
    AdapChol::AdapCholContext *m_context = AdapChol::allocateContext();
    AdapChol::setA(m_context, A);
    AdapChol::setBackend(m_context, cpuBackend, fpgaBackend);
    TIMED_RUN_REGION_START_ALWAYS(adapcholTime)
    AdapChol::run(m_context);
    AdapChol::postSolve(m_context, b2);
    TIMED_RUN_REGION_END_ALWAYS(adapcholTime)
    TIMED_RUN_REGION_START_ALWAYS(csparseTime)
    AdapChol::cs_cholsol(A, b1);
    TIMED_RUN_REGION_END_ALWAYS(csparseTime)
    for (int i = 0; i < n; i++) {
        csparseResultStream << b1[i] << '\n';
    }
    for (int i = 0; i < n; i++) {
        resultStream << b2[i] << '\n';
    }
    printf("adapchol time: %d, csparse time: %d, speed %.2fx\n", adapcholTime, csparseTime,
           (double) csparseTime / (double) adapcholTime);
    printf("adapchol mem pool used: %d\n", AdapChol::getMemPoolUsage(m_context));
}
