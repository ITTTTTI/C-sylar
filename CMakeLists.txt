cmake_minimum_required(VERSION 3.0)  # 指定 CMake 的最低版本要求

project(sylar)  # 设置项目名称

set(SMAKE_VERBOSE_MAKEFILE ON) #启用详细的 makefile 输出
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
# 这行代码设置 C++ 编译器的标志：

# $ENV{CXXFLAGS}：使用环境变量中的 CXXFLAGS。
# rdynamic：在 ELF 目标文件中，将所有符号导出到动态符号表。
# O3：优化级别 3，启用高级优化。
# fPIC：生成与位置无关的代码，用于共享库。
# ggdb：生成包含 GDB 调试信息的调试符号。
# std=c++11：使用 C++11 标准。
# Wall：启用所有警告。
# Wno-deprecated：禁用关于弃用功能的警告。
# Werror：将所有警告视为错误。
# Wno-unused-function：禁用未使用函数的警告。
# Wno-builtin-macro-redefined：禁用内置宏重定义的警告。
# Wno-deprecated-declarations：禁用弃用声明的警告。

include_directories(.)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)

find_library(YAMLCPP yaml-cpp)

set(LIB_SRC
    sylar/log.cc
    sylar/util.cc
    sylar/config.cc
    )
# 定义一个名为 LIB_SRC 的变量，其中包含了共享库的源文件 sylar/log.cc

add_library(sylar SHARED ${LIB_SRC}) #创建一个名为 sylar 的共享库（SHARED），源文件是 LIB_SRC 变量中的文件。
add_executable(test tests/test.cc)  #创建一个名为 test 的可执行文件，源文件是 tests/test.cc
add_dependencies(test sylar)  #指定 test 可执行文件依赖于 sylar 库。确保在构建 test 之前先构建 sylar 库。
target_link_libraries(test sylar yaml_cpp)  #指定 test 可执行文件链接 sylar 库。


# add_executable(test_config tests/test_config.cc)  #创建一个名为 test 的可执行文件，源文件是 tests/test.cc
# add_dependencies(test_config sylar)  #指定 test 可执行文件依赖于 sylar 库。确保在构建 test 之前先构建 sylar 库。
# target_link_libraries(test_config sylar yaml-cpp)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin) #可执行文件的输出路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)  #设置库文件的输出路径
# 添加其他库或依赖项，例如：
# find_package(...)

# 指定链接的库文件，例如：
# target_link_libraries(my_executable ...)
