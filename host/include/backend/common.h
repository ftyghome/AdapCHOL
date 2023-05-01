#pragma once

extern "C" {
#include "csparse/Include/cs.h"
}

namespace AdapChol {
    class AdapCholContext;

    class Backend {
    public:
        virtual void preProcessAMatrix(AdapChol::AdapCholContext &context) = 0;

        virtual void processColumns(AdapChol::AdapCholContext &context, int *tasks, int length) = 0;

        virtual void processAColumn(AdapChol::AdapCholContext &context, csi col) = 0;

        virtual bool *allocateP(size_t bytes) = 0;

        virtual void postProcessAMatrix(AdapChol::AdapCholContext &context) = 0;

        virtual int64_t getTimeCount() = 0;

        virtual void printStatistics() = 0;

        virtual void allocateAndFillL(AdapChol::AdapCholContext &context) = 0;
    };
}