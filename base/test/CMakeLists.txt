# cmake_minimum_required(VERSION 3.0)
# project(test)

# set(CMAKE_CXX_STANDARD 11)
# set(HOME E:/Study/cmw/base)
# set(EXECUTABLE_OUTPUT_PATH ${HOME}/bin/test/bin)
# include_directories(${HOME})

# file(GLOB SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# add_executable(test  ${SRC_LIST})

cmake_minimum_required(VERSION 3.0)
project(test_collection)

set(CMAKE_CXX_STANDARD 11)

# 1. 修正包含路径：指向 cmw 目录的上一级，解决头文件找不到的问题
set(INC_ROOT E:/Study)
include_directories(${INC_ROOT})

# 2. 设置所有 exe 的统一输出目录
set(EXECUTABLE_OUTPUT_PATH ${INC_ROOT}/cmw/base/test/bin)

# 3. 自动获取 src 目录下所有的 cpp 源文件
file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 4. 遍历每个源文件，分别为其创建 exe 目标
foreach(FILE_PATH ${SRC_FILES})
    # 从文件路径中提取文件名（不含后缀），例如从 "test01.cpp" 提取出 "test01"
    get_filename_component(FILE_NAME ${FILE_PATH} NAME_WE)
    
    # 为该文件创建一个可执行程序，名称就是文件名
    add_executable(${FILE_NAME} ${FILE_PATH})
    
    # 如果这些 test 还需要链接某些库，可以在这里添加：
    # target_link_libraries(${FILE_NAME} your_library_name)
    
    message(STATUS "Found source file: ${FILE_NAME}, target added.")
endforeach()