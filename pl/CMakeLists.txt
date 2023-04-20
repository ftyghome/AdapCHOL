# only for clangd to get syntax highlighting
cmake_minimum_required(VERSION 3.22)
project(my_application)

set(CMAKE_CXX_STANDARD 17)

include_directories(/home/gns/docker_data/jammy/workspace/apps/xilinx/Vitis_HLS/2022.2/include)

set(SOURCE_FILES src/krnl_col.cpp)

set(TEST_FILES testbench/test1.cpp)

add_library(kernel ${SOURCE_FILES})
add_library(kernel_test ${TEST_FILES})