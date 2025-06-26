# g00j小站 文件共享系统

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Language](https://img.shields.io/badge/language-C++-orange.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)

一个基于 C++ 和现代 Web 技术构建的轻量级文件共享系统，提供视频播放、文档阅读、文件管理、系统监控等完整功能。

## ✨ 特性

### 🔐 用户权限系统
- **游客权限**: 查看和预览共享文件
- **注册用户**: 文件上传、下载和个人文件管理
- **管理员权限**: 系统监控、用户管理、全局文件管理

### 📁 文件管理
- 支持多种文件类型（视频、图片、文档、压缩包等）
- 在线预览功能（视频播放、图片查看、文档阅读）
- 文件分类存储和组织
- 文件大小限制和类型验证
- 压缩文件在线解压

### 🎨 现代化界面
- 响应式设计，支持多设备访问
- Bootstrap 5 + 现代 CSS 样式
- 直观的用户交互体验
- 实时消息提示和反馈

### 📊 系统监控
- CPU、内存、磁盘使用率监控
- 进程列表和管理功能
- 网络状态监控
- 系统运行时间统计

### 🔒 安全特性
- 路径遍历攻击防护
- 文件类型白名单验证
- Session 认证机制
- CORS 跨域安全配置

## 🚀 快速开始

### 系统要求

- Linux 操作系统（推荐 Ubuntu 20.04+）
- GCC 7.0+ 支持 C++17
- SQLite 3
- jsoncpp 开发库

### 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential libsqlite3-dev libjsoncpp-dev

# CentOS/RHEL
sudo yum install gcc-c++ sqlite-devel jsoncpp-devel
```

### 编译和运行

```bash
# 克隆项目
git clone <项目地址>
cd g00j-file-share

# 一键构建
chmod +x build.sh
./build.sh

# 启动服务
chmod +x start.sh
./start.sh
```

### 访问系统

- **主页地址**: http://localhost:8080
- **默认管理员**: admin / admin123

## 📖 使用说明

### 基础使用

1. **游客访问**: 直接访问主页即可浏览和预览共享文件
2. **用户注册**: 点击"注册"按钮创建账户
3. **文件上传**: 登录后可上传文件到对应分类
4. **文件预览**: 支持视频、图片、文档的在线预览

### 管理功能

管理员登录后可以：
- 查看系统资源使用情况
- 管理系统进程
- 查看用户列表和文件统计
- 监控系统运行状态

### 文件分类

系统自动将文件分类存储：
- `videos/` - 视频文件（mp4, avi, mkv 等）
- `images/` - 图片文件（jpg, png, gif 等）
- `documents/` - 文档文件（txt, pdf, doc 等）
- `archives/` - 压缩文件（zip, tar, gz 等）

## 🏗️ 项目结构

```
g00j-file-share/
├── src/                    # C++ 源代码
│   ├── main.cpp           # 主程序入口
│   ├── server.cpp         # HTTP 服务器实现
│   ├── database.cpp       # 数据库操作
│   ├── file_manager.cpp   # 文件管理器
│   └── system_monitor.cpp # 系统监控
├── include/               # 头文件
├── static/                # 静态资源
│   ├── css/              # 样式文件
│   ├── js/               # JavaScript
│   └── index.html        # 主页模板
├── shared/                # 共享文件存储
├── build.sh              # 构建脚本
├── start.sh              # 启动脚本
├── Makefile              # Make 构建配置
├── CMakeLists.txt        # CMake 构建配置
└── README.md             # 项目文档
```

## 🔧 配置选项

### 文件上传限制

默认文件大小限制为 50MB，可以在 `include/file_manager.h` 中修改：

```cpp
bool check_file_size_limit(long size, long max_size = 50 * 1024 * 1024);
```

### 端口配置

默认端口为 8080，可以在 `src/main.cpp` 中修改：

```cpp
server = std::make_unique<HttpServer>(8080); // 修改为其他端口
```

### 数据库配置

数据库文件默认为 `g00j_share.db`，可以在 `src/main.cpp` 中修改。

## 🛠️ 开发

### 构建系统

项目支持两种构建方式：

1. **Makefile** (推荐)
```bash
make clean && make
```

2. **CMake**
```bash
mkdir build && cd build
cmake ..
make
```

### 添加新功能

1. 在相应的头文件中声明新方法
2. 在对应的 `.cpp` 文件中实现
3. 在 `main.cpp` 中注册新的路由
4. 更新前端界面（如需要）

### 调试

编译时添加调试信息：
```bash
make CXXFLAGS="-std=c++17 -Wall -Wextra -g -Iinclude"
```

## 📝 API 文档

### 用户认证

- `POST /api/login` - 用户登录
- `POST /api/register` - 用户注册
- `GET /api/user` - 获取当前用户信息

### 文件管理

- `GET /api/files` - 获取文件列表
- `POST /api/upload` - 上传文件
- `DELETE /api/files/:id` - 删除文件

### 系统监控

- `GET /api/system` - 获取系统信息
- `GET /api/processes` - 获取进程列表
- `POST /api/processes/:pid/kill` - 终止进程

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

### 贡献指南

1. Fork 项目
2. 创建特性分支
3. 提交更改
4. 推送到分支
5. 创建 Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🙏 致谢

- [Bootstrap](https://getbootstrap.com/) - 现代化 UI 框架
- [SQLite](https://www.sqlite.org/) - 轻量级数据库
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp) - JSON 处理库

## 📞 支持

如有问题或建议，请：
- 提交 [Issue](../../issues)
- 发送邮件至开发者
- 查看项目文档

---

**g00j小站** - 让文件共享更简单 ❤️ 