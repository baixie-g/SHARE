#!/bin/bash

# g00j小站 启动脚本

echo "=== g00j小站 文件共享系统 ==="
echo "正在启动服务器..."

# 检查可执行文件是否存在
if [ ! -f "./build/bin/g00j_file_share" ]; then
    echo "错误: 可执行文件不存在"
    echo "请先运行构建脚本: ./build.sh"
    exit 1
fi

# 创建必要的目录
echo "正在初始化目录结构..."

# 创建共享文件目录
mkdir -p shared/videos
mkdir -p shared/images  
mkdir -p shared/documents
mkdir -p shared/others

echo "✓ 共享文件目录已创建"

# 创建默认文档
if [ ! -f "shared/documents/README_API.md" ]; then
    cat > shared/documents/README_API.md << 'EOF'
# g00j小站 API 文档

## 简介

g00j小站是一个轻量级的文件共享系统，提供以下主要功能：

- 文件上传和下载
- 在线文件预览（视频、图片、文档）
- 用户管理和权限控制
- 系统监控（管理员权限）

## API 接口

### 用户认证

#### 登录
- **POST** `/api/login`
- 参数: `username`, `password`
- 返回: 成功后设置 session cookie

#### 注册
- **POST** `/api/register`
- 参数: `username`, `password`
- 返回: 注册结果

#### 登出
- **POST** `/api/logout`
- 清除 session cookie

### 文件管理

#### 获取文件列表
- **GET** `/api/files`
- 参数: `page` (页码), `limit` (每页数量), `category` (分类)
- 返回: 文件列表和分页信息

#### 文件上传
- **POST** `/api/upload`
- 需要登录权限
- 支持多文件上传

### 系统监控 (管理员权限)

#### 系统状态
- **GET** `/api/system/status`
- 返回: CPU、内存、磁盘使用率等信息

#### 进程列表
- **GET** `/api/system/processes`
- 返回: 系统进程列表

## 文件分类

- **videos**: 视频文件 (MP4, AVI, MKV, MOV, WMV, FLV)
- **images**: 图片文件 (JPG, PNG, GIF, BMP, WEBP)
- **documents**: 文档文件 (TXT, MD, PDF, DOC, DOCX, XLS, XLSX)
- **others**: 其他类型文件

## 默认用户

- **管理员**: admin / admin123
- **普通用户**: 需要注册

## 技术特性

- C++17 后端
- Vue3 前端
- SQLite3 数据库
- OpenSSL 安全加密
- 多线程处理
- 响应式 UI 设计

---

欢迎使用 g00j小站！
EOF
    echo "✓ API 文档已创建"
fi

if [ ! -f "shared/documents/welcome.txt" ]; then
    cat > shared/documents/welcome.txt << 'EOF'
欢迎使用 g00j小站 文件共享系统！

这是一个轻量级的文件共享平台，具有以下特点：

🎬 视频播放 - 支持 MP4, AVI, MKV 等多种格式在线播放
📄 文档阅读 - 支持 TXT, MD 等文本文件在线阅读  
🖼️ 图片预览 - 支持常见图片格式快速预览
📁 文件管理 - 上传、下载、分类管理多种文件类型
📊 系统监控 - 实时监控系统状态和进程信息（管理员功能）

安全特性：
- 用户认证和权限管理
- 文件类型安全验证
- 路径安全检查
- 文件大小限制

使用说明：
1. 游客可以查看和预览文件，下载需要登录
2. 注册用户可以上传和管理个人文件
3. 管理员具有系统监控和全局管理权限

默认管理员账户：admin / admin123

技术栈：
- 后端：C++17 + SQLite3 + OpenSSL
- 前端：Vue3 + Axios
- 构建：CMake
- 平台：Linux

享受您的文件共享体验！

--- g00j小站开发团队
EOF
    echo "✓ 欢迎文档已创建"
fi

echo
echo "正在启动服务器..."
echo "服务器地址: http://localhost:8080"
echo "默认管理员: admin / admin123"
echo
echo "按 Ctrl+C 停止服务器"
echo

# 启动服务器
./build/bin/g00j_file_share 