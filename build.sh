#!/bin/bash

# g00j小站 构建脚本

echo "=== g00j小站 文件共享系统 构建脚本 ==="
echo "正在检查系统依赖..."

# 检查必要的依赖
check_dependency() {
    if command -v $1 >/dev/null 2>&1; then
        echo "✓ $1 已安装"
        return 0
    else
        echo "✗ $1 未安装"
        return 1
    fi
}

# 检查 cmake
if ! check_dependency cmake; then
    echo "请安装 cmake: sudo apt install cmake"
    exit 1
fi

# 检查 g++
if ! check_dependency g++; then
    echo "请安装 g++: sudo apt install g++"
    exit 1
fi

# 检查 pkg-config
if ! check_dependency pkg-config; then
    echo "请安装 pkg-config: sudo apt install pkg-config"
    exit 1
fi

# 检查 SQLite3 开发库
if ! pkg-config --exists sqlite3; then
    echo "✗ SQLite3 开发库未安装"
    echo "请安装: sudo apt install libsqlite3-dev"
    exit 1
else
    echo "✓ SQLite3 开发库已安装"
fi

# 检查 OpenSSL 开发库
if ! pkg-config --exists openssl; then
    echo "✗ OpenSSL 开发库未安装"
    echo "请安装: sudo apt install libssl-dev"
    exit 1
else
    echo "✓ OpenSSL 开发库已安装"
fi

echo
echo "所有依赖检查完成，开始构建..."

# 创建构建目录
if [ ! -d "build" ]; then
    mkdir build
    echo "✓ 创建 build 目录"
fi

# 创建 bin 目录
if [ ! -d "bin" ]; then
    mkdir bin
    echo "✓ 创建 bin 目录"
fi

# 进入构建目录
cd build

echo
echo "正在配置项目..."
if cmake ..; then
    echo "✓ CMake 配置成功"
else
    echo "✗ CMake 配置失败"
    exit 1
fi

echo
echo "正在编译项目..."
if make -j$(nproc); then
    echo "✓ 编译成功"
else
    echo "✗ 编译失败"
    exit 1
fi

# 返回项目根目录
cd ..

echo
echo "=== 构建完成 ==="
echo "可执行文件: ./bin/g00j_file_share"
echo "静态文件目录: ./static/"
echo "共享文件目录: ./shared/"
echo
echo "运行命令: ./start.sh"
echo "或者直接运行: ./bin/g00j_file_share" 