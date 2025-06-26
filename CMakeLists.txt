cmake_minimum_required(VERSION 3.10)
project(g00j_file_share)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找依赖库
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
find_package(OpenSSL REQUIRED)

# 包含目录
include_directories(include)
include_directories(${SQLITE3_INCLUDE_DIRS})

# 源文件
set(SOURCES
    src/main.cpp
    src/server.cpp
    src/database.cpp
    src/file_manager.cpp
    src/json_helper.cpp
    src/system_monitor.cpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME} 
    ${SQLITE3_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

# 编译选项
target_compile_options(${PROJECT_NAME} PRIVATE ${SQLITE3_CFLAGS_OTHER})

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2") 