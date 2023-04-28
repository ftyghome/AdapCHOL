#pragma once

extern "C" {
#include "csparse/Include/cs.h"
}

#include "backend/common.h"
#include "adapchol.h"

#include <cstdint>
#include <cmath>


namespace AdapChol {
    class AdapCholContext;

    class CPUBackend : public Backend {
    public:

        void processAColumn(AdapChol::AdapCholContext &context, csi col);

        void preProcessAMatrix(AdapChol::AdapCholContext &context) override;

        void postProcessAMatrix(AdapChol::AdapCholContext &context) override;

        static void Sqrt_Div(double *F, csi Fn, double *L);

        static void Sqrt_Div_Leaf(int64_t Fn, double *Lx);

        static void Gen_Update_Matrix(const double *F, double *U, csi Fn);

        static void Gen_Update_Matrix_Leaf(const double *F, double *U, csi Fn);

        static void Gen_Update_Matrix_And_Write_Direct(const double *descF, double *parF,
                                                       const bool *P, csi descFn, csi parFn);

        static void Gen_Update_Matrix_And_Write_Direct_Leaf(const double *descF, double *parF,
                                                            const bool *P, csi descFn, csi parFn);

        static void Extern_Add(double *dest_F, const double *U, const bool *P, csi Fn);

        static void Result_Write(const double *F, double *Cx, csi Fn);

        static double *getFMemFromPool(AdapChol::AdapCholContext &context);

        static void returnFMemToPool(AdapChol::AdapCholContext &context, double *mem);

        bool *allocateP(size_t bytes) override;

        int64_t getTimeCount() override;

        void printStatistics() override;

        void allocateAndFillL(AdapChol::AdapCholContext &context) override;
    };
}
