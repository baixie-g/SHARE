cmake_minimum_required(VERSION 3.16)
project(112_file_share)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 查找依赖库
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# 查找OpenSSL
find_package(OpenSSL REQUIRED)

# 包含头文件目录
include_directories(include)

# 设置源文件
set(SOURCES
    src/main.cpp
    src/server.cpp
    src/database.cpp
    src/file_manager.cpp
    src/json_helper.cpp
    src/system_monitor.cpp
)

# 创建可执行文件
add_executable(112_file_share ${SOURCES})

# 链接库
target_link_libraries(112_file_share 
    ${SQLITE3_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

# 编译选项
target_compile_options(112_file_share PRIVATE ${SQLITE3_CFLAGS_OTHER}) 