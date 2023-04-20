cmake_minimum_required(VERSION 3.0)
project(csparse)

set(CMAKE_CXX_STANDARD 17)

include_directories(Include)

AUX_SOURCE_DIRECTORY(Source CSPARSE_SOURCE)

add_library(csparse ${CSPARSE_SOURCE})
