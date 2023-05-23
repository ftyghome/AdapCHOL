#include <fstream>
#include <cassert>
#include <chrono>
#include <functional>
#include "adapchol.h"
#include "internal/utils.h"

int main(int args, char *argv[]) {
    std::ofstream resultStream(argv[1]);
    std::ofstream csparseResultStream(argv[2]);
    std::ofstream frontalStream(argv[3]), updateStream(argv[4]), pStream(argv[5]);
    int64_t adapcholTime = 0, csparseTime = 0;
    cs *A = AdapChol::loadSparse(stdin);

    auto *cpuBackend = AdapChol::allocateCPUBackend();

#if defined(__x86_64__) || defined(_M_X64)
    AdapChol::FPGABackend *fpgaBackend = nullptr;
#else
    AdapChol::FPGABackend *fpgaBackend = AdapChol::allocateFPGABackend(std::string("/lib/firmware/xilinx/adapchol/binary_container_1.bin"), 4);
#endif
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
    printf("adapchol time: %ld, csparse time: %ld, speed %.2fx\n", adapcholTime, csparseTime,
           (double) csparseTime / (double) adapcholTime);
    printf("adapchol mem pool used: %d\n", AdapChol::getMemPoolUsage(m_context));
}
