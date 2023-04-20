#include <fstream>
#include <cassert>
#include <iostream>
#include <cmath>
#include "../include/krnl_col.h"

const int MAX_ELEM = 100;

double descF[100] = {0, 0, 0, 0, 0, 0};
double parF[100] = {0, 0, 0, 0, 0, 0};
double L[100] = {5, 1, 2};
bool P[100] = {true, false, true, false, false, true};

int main(int argc, char *argv[]) {

//    bool P[] = {false, false, false, false, false, false};
    int64_t descFn = 3, parFn = 3;
    krnl_proc_col(descF, L, P, parF, descFn, parFn);
    double targetParF[] = {-0.2, 0, -0.4, 0, 0, -0.8};

    for (double i: parF) {
        std::cout << i << " ";
    }
    std::cout << std::endl;

    for (int i = 0; i < 6; i++) {
        assert(fabs((parF[i] - targetParF[i])) <= 1e-4);
    }
}