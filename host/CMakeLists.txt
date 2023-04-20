cmake_minimum_required(VERSION 3.22)
project(adapchol_host)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(thirdparty/csparse)

set(SOURCE_FILES src/adapchol.cpp)

include_directories(include thirdparty)

add_library(adapchol_host ${SOURCE_FILES})

add_library(adapchol_cpu_backend src/backend/cpu/cpu.cpp)

add_library(adapchol_fpga_backend src/backend/fpga/fpga.cpp src/backend/fpga/xrt_wrapper.cpp)

set(BACKENDS adapchol_cpu_backend adapchol_fpga_backend)

target_link_libraries(adapchol_host ${BACKENDS} csparse)

target_include_directories(adapchol_fpga_backend PUBLIC thirdparty/XRT/include)

add_executable(test_host test/src/test.cpp test/src/io.cpp)
target_include_directories(test_host PUBLIC test/include)
target_link_libraries(test_host adapchol_host)
