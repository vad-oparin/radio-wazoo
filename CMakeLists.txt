cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include_directories(${CMAKE_SOURCE_DIR}/include)

#set(CMAKE_C_CLANG_TIDY "clang-tidy;-checks=*;-header-filter=.*")

project(radio-wazoo)
