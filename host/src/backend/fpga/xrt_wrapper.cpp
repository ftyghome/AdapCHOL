//
// Created by gns on 4/20/23.
//
#include "backend/fpga/xrt_wrapper.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_bo.h"
#include "xrt/xrt_kernel.h"

DeviceContext::DeviceContext(const std::string &binaryFile, int deviceIndex) {
    device = std::make_shared<xrt::device>(new xrt::device(deviceIndex));
    uuid = std::make_shared<xrt::uuid>(device->load_xclbin(binaryFile));
}

xrt::kernel DeviceContext::getKernel(const std::string &kernelName) {
    return {*device, *uuid, kernelName};
}

