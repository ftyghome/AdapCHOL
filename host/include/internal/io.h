#pragma once

#include <iostream>
#include <fstream>
#include <cstring>

extern "C" {
#include "csparse/Include/cs.h"
}

namespace AdapChol {
    void printCS(const cs *mat);

    void dumpFormalResult(std::ofstream &stream, cs *mat);

    template<typename T>
    void printDenseTrig(const T *F, int64_t size, std::ostream &stream);
}