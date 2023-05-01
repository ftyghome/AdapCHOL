//
// Created by gns on 4/20/23.
//
#include "xrt/xrt_uuid.h"
#include "backend/fpga/xrt_wrapper.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_bo.h"
#include "xrt/xrt_kernel.h"
#include <iostream>

DeviceContext::DeviceContext(const std::string &binaryFile, int deviceIndex) {
    device = new xrt::device(deviceIndex);
    uuid = device->load_xclbin(binaryFile);
}

xrt::kernel DeviceContext::getKernel(const std::string &kernelName) {
    return {*device, uuid, kernelName};
}

DevicePtr DeviceContext::getDevice() const {
    return device;
}

