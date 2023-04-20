#include "adapchol.h"
#include "io.h"
#include <fstream>
#include <cassert>
#include <chrono>
#include <functional>

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
    AdapChol::AdapCholContext m_context(A);
    auto adapcholTime = timedRun([&] {
        m_context.run();
    });
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
