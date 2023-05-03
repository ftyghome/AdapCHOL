#include "adapchol.h"
#include "internal/io.h"
#include <fstream>
#include <cassert>
#include <chrono>
#include <functional>
#include "backend/cpu/cpu.h"
#include "backend/fpga/fpga.h"
#include "utils.h"


int main(int args, char *argv[]) {
    std::ofstream resultStream(argv[1]);
    std::ofstream csparseResultStream(argv[2]);
    std::ofstream frontalStream(argv[3]), updateStream(argv[4]), pStream(argv[5]);
    int64_t adapcholTime = 0, csparseTime = 0;
    cs *A = cs_compress(cs_load(stdin));

    auto *cpuBackend = new AdapChol::CPUBackend();

#if defined(__x86_64__) || defined(_M_X64)
    AdapChol::Backend *fpgaBackend = nullptr;
#else
    AdapChol::Backend *fpgaBackend = new AdapChol::FPGABackend(std::string("/lib/firmware/xilinx/adapchol/binary_container_1.bin"), 4);
#endif
    AdapChol::AdapCholContext m_context;
    m_context.setA(A);
    m_context.setBackend(cpuBackend, fpgaBackend);
    TIMED_RUN_REGION_START_ALWAYS(adapcholTime)
    m_context.run();
    TIMED_RUN_REGION_END_ALWAYS(adapcholTime)
    cs *result = m_context.getResult();
    dumpFormalResult(resultStream, m_context.getResult());
    csn *csparse_result;
    TIMED_RUN_REGION_START_ALWAYS(csparseTime)
    css *symbol = cs_schol(1, A);
    csparse_result = cs_chol(A, symbol);
    TIMED_RUN_REGION_END_ALWAYS(csparseTime)
    dumpFormalResult(csparseResultStream, csparse_result->L);
    printf("adapchol time: %ld, csparse time: %ld, speed %.2fx\n", adapcholTime, csparseTime,
           (double) csparseTime / (double) adapcholTime);
    printf("adapchol mem pool used: %d\n", m_context.getMemPoolUsage());
}
