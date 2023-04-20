#pragma once

#if defined(__x86_64__) || defined(_M_X64)
#define DISABLE_ABI_CHECK
#endif

#include <memory>
#include <string>

namespace xrt {
    class device;

    class uuid;

    class kernel;
}

using DevicePtr = std::shared_ptr<xrt::device>;
using UUIDPtr = std::shared_ptr<xrt::uuid>;
using KernelPtr = std::shared_ptr<xrt::kernel>;


class DeviceContext {
private:
    DevicePtr device;
    UUIDPtr uuid;

public:
    DeviceContext(const std::string &binaryFile, int deviceIndex);

    xrt::kernel getKernel(const std::string &kernelName);
};