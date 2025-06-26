#!/bin/bash

# g00j小站 文件共享系统 - 启动脚本

echo "=== g00j小站启动脚本 ==="

# 检查可执行文件是否存在
if [ ! -f "bin/g00j_file_share" ]; then
    echo "错误: 未找到可执行文件 bin/g00j_file_share"
    echo "请先运行 ./build.sh 构建项目"
    exit 1
fi

# 检查并创建必要的目录
echo "检查目录结构..."
mkdir -p bin
mkdir -p shared/videos
mkdir -p shared/documents
mkdir -p shared/images
mkdir -p shared/others
mkdir -p static

# 创建默认的欢迎文档
if [ ! -f "shared/documents/welcome.txt" ]; then
    echo "创建默认欢迎文档..."
    cat > shared/documents/welcome.txt << EOF
欢迎使用 g00j小站 文件共享系统！

这是一个基于C++和Vue3开发的轻量级文件共享平台，具有以下特性：

## 功能特性
- 🔐 用户权限管理（游客、注册用户、管理员）
- 📁 文件上传、下载、在线预览
- 🎥 视频在线播放
- 📄 文档在线阅读
- 🖼️ 图片预览
- 📊 系统资源监控
- 🔍 文件搜索功能

## 默认账户
- 管理员: admin / admin123
- 普通用户: 需要注册

## 访问地址
- 主页: http://localhost:8080
- 管理面板: 登录后可见

祝您使用愉快！
EOF
fi

# 创建API文档
if [ ! -f "shared/documents/README_API.md" ]; then
    echo "创建API文档..."
    cat > shared/documents/README_API.md << EOF
# g00j小站 API 文档

## 用户认证

### 用户登录
- **POST** /api/login
- **请求体**: {"username": "用户名", "password": "密码"}
- **响应**: 用户信息和会话Cookie

### 用户注册
- **POST** /api/register
- **请求体**: {"username": "用户名", "password": "密码"}
- **响应**: 注册结果

### 用户登出
- **POST** /api/logout
- **响应**: 登出结果，清除会话

## 文件管理

### 获取文件列表
- **GET** /api/files
- **参数**: 
  - limit: 每页数量 (默认50)
  - offset: 偏移量 (默认0)
  - category: 文件分类 (可选)
- **响应**: 文件列表

### 文件上传
- **POST** /api/upload
- **请求体**: multipart/form-data
- **响应**: 上传结果

### 文件下载
- **GET** /api/download/[文件ID]
- **响应**: 文件内容

## 系统监控

### 获取系统状态
- **GET** /api/system/status
- **响应**: CPU、内存、磁盘使用情况

### 获取进程列表
- **GET** /api/system/processes
- **响应**: 系统进程列表（需要管理员权限）

## 错误代码

- 200: 成功
- 400: 请求格式错误
- 401: 未授权
- 403: 权限不足
- 404: 资源不存在
- 409: 资源冲突
- 500: 服务器内部错误
EOF
fi

echo "正在启动服务器..."
echo "如需停止服务器，请按 Ctrl+C"
echo ""

# 启动服务器
cd bin
./g00j_file_share 