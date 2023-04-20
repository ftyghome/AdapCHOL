#pragma once

extern "C" {
#include "csparse/Include/cs.h"
}

namespace AdapChol {
    class AdapCholContext;

    class Backend {
    public:
        virtual void processAColumn(AdapChol::AdapCholContext &context, csi col) = 0;
    };
}