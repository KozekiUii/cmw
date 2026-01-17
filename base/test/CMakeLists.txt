cmake_minimum_required(VERSION 3.0)
project(test)

set(CMAKE_CXX_STANDARD 11)
set(HOME E:/Study/cmw/base)
set(EXECUTABLE_OUTPUT_PATH ${HOME}/bin/test/bin)
include_directories(${HOME})

file(GLOB SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(test  ${SRC_LIST})
