#if defined(__x86_64__) || defined(_M_X64)
#define DISABLE_ABI_CHECK
#endif

#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"
#include <iostream>

void *mallocAligned(size_t bytes) {
    void *host_ptr;
    int size = posix_memalign(&host_ptr, 4096, bytes);
    return host_ptr;
}

int main(int argc, char *argv[]) {
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin("/lib/firmware/xilinx/adapchol/binary_container_1.bin");
    auto descF = (double *) mallocAligned(100 * sizeof(double));
    auto parF = (double *) mallocAligned(100 * sizeof(double));
    auto P = (bool *) mallocAligned(100 * sizeof(bool));
    for (int i = 0; i < 100; i++) {
        descF[i] = parF[i] = P[i] = 0;
    }
    descF[0] = 0.8;
    descF[1] = 2.0;
    descF[2] = -0.4;
    descF[5] = -0.8;
    P[0] = P[1] = P[3] = true;
    auto kernel = xrt::kernel(device, uuid, "krnl_proc_col");
//    auto descF_Buffer = xrt::bo(device, descF, 0 * sizeof(double), XRT_BO_FLAGS_CACHEABLE, kernel.group_id(0));
//    auto P_Buffer = xrt::bo(device, P, 0 * sizeof(bool), XRT_BO_FLAGS_CACHEABLE, kernel.group_id(1));
//    auto parF_Buffer = xrt::bo(device, parF, 100 * sizeof(double), XRT_BO_FLAGS_CACHEABLE, kernel.group_id(2));
    const size_t testing_size = 8;
//    auto *testing_buffer = (unsigned char *) mallocAligned(testing_size);
//    auto parF_Buffer = xrt::bo(device, testing_buffer, testing_size, xrt::bo::flags::host_only, kernel.group_id(2));
    std::cout << kernel.group_id(0) << " " << kernel.group_id(1) << " " << kernel.group_id(2) << std::endl;
//    descF_Buffer.sync(XCL_BO_SYNC_BO_TO_DEVICE);
//    P_Buffer.sync(XCL_BO_SYNC_BO_TO_DEVICE);
//    parF_Buffer.sync(XCL_BO_SYNC_BO_TO_DEVICE);
//
//    auto run = kernel(descF_Buffer, P_Buffer, parF_Buffer, 3, 3);
//    run.wait();
//
//    parF_Buffer.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
//
//    for (int i = 0; i < 6; i++) {
//        printf("%.2lf ", parF[i]);
//    }

}