# 112小站 文件共享系统

🏠 一个轻量级的文件共享平台，基于 C++ 后端和 Vue3 前端开发。

## ✨ 主要特性

### 🎬 多媒体支持
- **视频播放**: 支持 MP4, AVI, MKV, MOV, WMV, FLV 格式在线播放
- **图片预览**: 支持 JPG, PNG, GIF, BMP, WEBP 格式快速预览
- **文档阅读**: 支持 TXT, MD, PDF, DOC, DOCX 等文档在线阅读

### 👥 用户权限管理
- **游客权限**: 浏览和预览文件，下载需要登录
- **注册用户**: 文件上传下载、个人文件管理
- **管理员权限**: 系统监控、进程管理、全局文件管理

### 📊 系统监控
- **实时监控**: CPU、内存、磁盘使用率
- **进程管理**: 查看系统进程列表，支持进程终止
- **系统信息**: 系统负载、运行时间等详细信息

### 🔒 安全特性
- **用户认证**: 基于 Session 的用户认证系统
- **文件安全**: 文件类型白名单、路径安全检查
- **权限控制**: 三级权限分离，管理员权限保护
- **密码加密**: SHA256 哈希加密存储

## 🛠️ 技术栈

- **后端**: C++17, SQLite3, OpenSSL
- **前端**: Vue3, Axios, CSS3
- **构建**: CMake, Make
- **部署**: Linux (Ubuntu/Debian)

## 📋 系统要求

- Linux 操作系统 (Ubuntu 18.04+ 或 Debian 9+)
- CMake 3.16+
- GCC 8.0+ (支持 C++17)
- SQLite3 开发库
- OpenSSL 开发库

## 🚀 快速开始

### 1. 安装依赖

```bash
# Ubuntu/Debian
make install

# 或手动安装
sudo apt update
sudo apt install cmake g++ pkg-config libsqlite3-dev libssl-dev
```

### 2. 构建项目

```bash
make build
# 或
./build.sh
```

### 3. 启动服务器

```bash
make start
# 或
./start.sh
```

### 3.1 后台启动服务器（推荐）

```bash
# 项目根目录
sudo ./service_control.sh start     # 启动
sudo ./service_control.sh status    # 查看运行状态
sudo ./service_control.sh stop      # 停止
sudo ./service_control.sh restart   # 重启
```

> 该脚本使用 `nohup` 将程序以守护进程方式运行，SSH 断开后依然保持运行。
>
> - **PID 文件**: `/var/run/112_file_share.pid`
> - **日志文件**: `/var/log/112_file_share.log`

### 4. 访问系统

打开浏览器访问 [http://localhost:80](http://localhost:80)

**默认管理员账户**: `admin` / `admin123`

## 📁 目录结构

```
112小站/
├── src/                    # C++ 源代码
│   ├── main.cpp           # 主程序入口
│   ├── server.cpp         # HTTP 服务器
│   ├── database.cpp       # 数据库管理
│   ├── file_manager.cpp   # 文件管理
│   ├── json_helper.cpp    # JSON 处理
│   └── system_monitor.cpp # 系统监控
├── include/               # 头文件
├── static/                # 前端静态文件
│   ├── index.html        # 主页面
│   ├── css/style.css     # 样式文件
│   └── js/app.js         # Vue3 应用
├── shared/                # 共享文件存储
│   ├── videos/           # 视频文件
│   ├── images/           # 图片文件
│   ├── documents/        # 文档文件
│   └── others/           # 其他文件
├── build.sh              # 构建脚本
├── start.sh              # 启动脚本
├── CMakeLists.txt        # CMake 配置
├── Makefile              # Make 配置
└── README.md             # 项目说明
```

## 🎯 功能说明

### 文件管理
- 支持多文件上传，拖拽上传
- 自动文件分类（视频、图片、文档、其他）
- 文件安全检查和大小限制（默认 50MB）
- 分页显示，支持按分类筛选

### 用户系统
- 用户注册和登录
- 基于 Cookie 的会话管理
- 三级权限控制（游客、用户、管理员）

### 系统监控
- 实时 CPU、内存、磁盘使用率
- 系统进程列表和管理
- 系统负载和运行时间监控

## 🔧 配置选项

### 服务器配置
- **端口**: 80 (可在源码中修改)
- **最大文件大小**: 50MB
- **数据库文件**: `bin/112_share.db`

### 支持的文件类型
- **视频**: mp4, avi, mkv, mov, wmv, flv
- **图片**: jpg, jpeg, png, gif, bmp, webp
- **文档**: txt, md, pdf, doc, docx, xls, xlsx
- **压缩**: zip, rar, 7z
- **音频**: mp3, wav, flac

## 📚 API 文档

### 用户认证
- `POST /api/login` - 用户登录
- `POST /api/register` - 用户注册
- `POST /api/logout` - 用户登出

### 文件管理
- `GET /api/files` - 获取文件列表
- `POST /api/upload` - 文件上传

### 系统监控 (管理员)
- `GET /api/system/status` - 系统状态
- `GET /api/system/processes` - 进程列表

## 🐛 常见问题

### 构建失败
- 确保已安装所有依赖库
- 检查 GCC 版本是否支持 C++17
- 确保有足够的磁盘空间

### 启动失败
- 检查端口 80 是否被占用
- 确保有创建文件和目录的权限
- 查看错误日志定位问题

### 文件上传失败
- 检查文件大小是否超出限制
- 确认文件类型是否在白名单中
- 检查磁盘空间是否充足

## 🤝 贡献指南

1. Fork 项目
2. 创建功能分支
3. 提交更改
4. 推送到分支
5. 创建 Pull Request

## 📄 许可证

本项目采用 MIT 许可证，详见 LICENSE 文件。

## 👨‍💻 开发团队

**112小站开发团队** - 致力于创建简单、高效的文件共享解决方案。

---

⭐ 如果这个项目对您有帮助，请给我们一个 Star！ 