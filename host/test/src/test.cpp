#include "adapchol.h"
#include "io.h"
#include <fstream>
#include <cassert>
#include <chrono>
#include <functional>
#include "backend/cpu/cpu.h"
#include "backend/fpga/fpga.h"


auto timedRun(const std::function<void(void)> &func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return duration.count();
}

int main(int args, char *argv[]) {
    std::ofstream resultStream(argv[1]);
    std::ofstream csparseResultStream(argv[2]);
    std::ofstream frontalStream(argv[3]), updateStream(argv[4]), pStream(argv[5]);
    cs *A = cs_compress(cs_load(stdin));

    auto *cpuBackend = new AdapChol::CPUBackend();

#if defined(__x86_64__) || defined(_M_X64)
    AdapChol::Backend *fpgaBackend = nullptr;
#else
    AdapChol::Backend *fpgaBackend = new AdapChol::FPGABackend(std::string("/lib/firmware/xilinx/adapchol/binary_container_1.bin"));
#endif
    AdapChol::AdapCholContext m_context;
    m_context.setA(A);
    m_context.setBackend(cpuBackend, fpgaBackend);
    auto adapcholTime = timedRun([&] {
        m_context.run();
    });
    cs *result = m_context.getResult();
    dumpFormalResult(resultStream, m_context.getResult());
    csn *csparse_result;
    auto csparseTime = timedRun([&] {
        css *symbol = cs_schol(1, A);
        csparse_result = cs_chol(A, symbol);
    });
    dumpFormalResult(csparseResultStream, csparse_result->L);
    printf("adapchol time: %ld, csparse time: %ld, speed %.2fx\n", adapcholTime, csparseTime,
           (double) csparseTime / (double) adapcholTime);
    printf("adapchol mem pool used: %d\n", m_context.getMemPoolUsage());
}
