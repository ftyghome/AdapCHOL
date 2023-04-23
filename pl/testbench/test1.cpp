#include <fstream>
#include <cassert>
#include <iostream>
#include <cmath>
#include "../include/krnl_col.h"

const int MAX_ELEM = 100;

double descF[100] = {0.8, 2.0, -0.4, 0, 0, -0.8};
double parF[100] = {0, 0, 0, 0, 0, 0};
bool P[100] = {true, true, false, true, false, false};

int main(int argc, char *argv[]) {

//    bool P[] = {false, false, false, false, false, false};
    int64_t descFn = 3, parFn = 3;
    krnl_proc_col(descF, P, parF, descFn, parFn);

    for (double i: parF) {
        std::cout << i << " ";
    }
    std::cout << std::endl;

}
