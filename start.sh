#!/bin/bash

echo "=== g00j小站 文件共享系统 ==="

# 检查是否已构建
if [ ! -f "bin/g00j_file_share" ]; then
    echo "项目未构建，正在构建..."
    ./build.sh
fi

# 创建必要目录
mkdir -p shared/videos shared/articles shared/images shared/documents

# 检查端口是否被占用
if lsof -i:8080 > /dev/null 2>&1; then
    echo "警告: 端口 8080 已被占用"
    echo "你可能需要以管理员权限运行或更改端口"
fi

echo "启动服务器..."
echo "访问地址: http://localhost:8080"
echo "按 Ctrl+C 停止服务"
echo ""

# 启动服务器
cd bin && ./g00j_file_share 