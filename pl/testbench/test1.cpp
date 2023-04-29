#include <fstream>
#include <cassert>
#include <iostream>
#include <cmath>
#include <random>
#include "../include/krnl_col.h"

const int DESC_ELEM = 20000;
const int PAR_ELEM = 50000;

double descF[DESC_ELEM];
double parF[PAR_ELEM];
bool P[PAR_ELEM] = {};
double L[DESC_ELEM];

int main(int argc, char *argv[]) {
    std::mt19937 eng(2023);
    std::uniform_real_distribution<double> dis(0.1, 2.0);

//    bool P[] = {false, false, false, false, false, false};
    int64_t descFn = 100, parFn = 120;
    int64_t descFSize = (1 + descFn) * descFn / 2, parFSize = (1 + parFn) * parFn / 2;
    for (int i = 0; i < descFSize; i++) {
        descF[i] = dis(eng);
    }
    for (int i = 0; i < parFSize; i++) {
        parF[i] = dis(eng);
    }
    for (int i = 0; i < descFn; i++) {
        L[i] = dis(eng);
    }
    for (int i = 0; i < descFn - 1; i++) {
        P[i] = true;
    }


    krnl_proc_col(descF, P, parF, L, 0, (int) descFn, (int) parFn, 0b00000000);

    for (double i: parF) {
        std::cout << i << " ";
    }
    std::cout << std::endl;

}
