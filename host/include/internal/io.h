#pragma once

#include <iostream>
#include <fstream>
#include <cstring>
#include "adapchol.h"

extern "C" {
#include "csparse/Include/cs.h"
}

void printCS(const cs *mat);

void dumpFormalResult(std::ofstream &stream, cs *mat);

template<typename T>
void printDenseTrig(const T *F, int64_t size, std::ostream &stream);