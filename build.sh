#!/bin/bash

echo "=== g00j小站 文件共享系统构建脚本 ==="

# 检查依赖
echo "检查系统依赖..."
if ! pkg-config --exists sqlite3; then
    echo "错误: 未找到 SQLite3 开发库"
    echo "请安装: sudo apt-get install libsqlite3-dev"
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "错误: 未找到 g++ 编译器"
    echo "请安装: sudo apt-get install build-essential"
    exit 1
fi

# 创建必要目录
echo "创建项目目录..."
mkdir -p obj bin shared/videos shared/articles shared/images shared/documents

# 构建项目
echo "开始构建..."
if make clean && make; then
    echo "构建成功！"
    
    # 设置权限
    chmod +x bin/g00j_file_share
    chmod +x start.sh
    
    echo "=== 构建完成 ==="
    echo "运行命令: ./start.sh"
    echo "访问地址: http://localhost:8080"
    echo "默认管理员: admin / admin123"
else
    echo "构建失败！"
    exit 1
fi 