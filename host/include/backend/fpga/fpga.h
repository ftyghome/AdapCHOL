#pragma once

extern "C" {
#include "csparse/Include/cs.h"
}

#include "backend/common.h"
#include "xrt_wrapper.h"
#include <cstdint>
#include <cmath>

namespace AdapChol {
    class AdapCholContext;

    class FPGABackend : public Backend {
    private:
        DeviceContext deviceContext;
        KernelPtr kernel;
    public:
        FPGABackend(const std::string &binaryFile);

        void processAColumn(AdapChol::AdapCholContext &context, csi col);

        static void Sqrt_Div(double *F, csi Fn, double *L);

        static void Sqrt_Div_Leaf(int64_t Fn, double *Lx);

        static void Gen_Update_Matrix(const double *F, double *U, csi Fn);

        static void Gen_Update_Matrix_Leaf(const double *F, double *U, csi Fn);

        static void Gen_Update_Matrix_And_Write_Direct(const double *descF, double *parF,
                                                       const bool *P, csi descFn);

        static void Gen_Update_Matrix_And_Write_Direct_Leaf(const double *descF, double *parF,
                                                            const bool *P, csi descFn);

        static void Extern_Add(double *dest_F, const double *U, const bool *P, csi Fn);

        static void Result_Write(const double *F, double *Cx, csi Fn);
    };
}
