cmake_minimum_required(VERSION 3.22)
project(IOS2 C)

set(CMAKE_C_STANDARD 99)
execute_process(COMMAND make -f ${CMAKE_CURRENT_SOURCE_DIR}/Makefile)
add_executable(IOS2 proj2.c)
