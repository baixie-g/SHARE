#!/bin/bash

# g00j小站 文件共享系统 - 构建脚本
# 编译C++后端服务器

echo "=== g00j小站构建脚本 ==="
echo "正在构建项目..."

# 检查必要的工具
echo "检查构建工具..."
if ! command -v cmake &> /dev/null; then
    echo "错误: 未找到cmake，请先安装cmake"
    exit 1
fi

if ! command -v pkg-config &> /dev/null; then
    echo "错误: 未找到pkg-config，请先安装pkg-config"
    exit 1
fi

# 检查SQLite3开发库
echo "检查SQLite3开发库..."
if ! pkg-config --exists sqlite3; then
    echo "错误: 未找到SQLite3开发库，请安装libsqlite3-dev"
    echo "Ubuntu/Debian: sudo apt-get install libsqlite3-dev"
    echo "CentOS/RHEL: sudo yum install sqlite-devel"
    exit 1
fi

# 创建必要的目录
echo "创建目录结构..."
mkdir -p bin
mkdir -p obj
mkdir -p shared/videos
mkdir -p shared/documents
mkdir -p shared/images
mkdir -p shared/others
mkdir -p static

# 使用CMake构建项目
echo "使用CMake构建项目..."
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# 生成构建文件
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译项目
make -j$(nproc)

# 检查编译是否成功
if [ $? -eq 0 ]; then
    echo "项目构建成功！"
    
    # 复制可执行文件到bin目录
    cd ..
    cp build/bin/g00j_file_share bin/
    
    echo "可执行文件已复制到 bin/g00j_file_share"
    echo "使用 ./start.sh 启动服务器"
else
    echo "项目构建失败！"
    exit 1
fi

echo "构建完成！" 