#pragma once

#if defined(__x86_64__) || defined(_M_X64)
#define DISABLE_ABI_CHECK
#endif

#define FPGA_MEM_BANK_ID 2

#include <memory>
#include <string>

namespace xrt {
    class device;

    class uuid;

    class kernel;

    class bo;

    class run;
}

using DevicePtr = xrt::device *;
using UUIDPtr = xrt::uuid *;
using KernelPtr = xrt::kernel *;
using BoPtr = xrt::bo *;
using RunPtr = xrt::run *;


class DeviceContext {
private:
    DevicePtr device;
    UUIDPtr uuid;

public:
    DeviceContext(const std::string &binaryFile, int deviceIndex);

    xrt::kernel getKernel(const std::string &kernelName);

    [[nodiscard]] DevicePtr getDevice() const;
};