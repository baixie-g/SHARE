# g00j小站 文件共享系统

一个基于C++后端和Vue3前端的轻量级文件共享系统，提供文件上传、下载、在线预览、用户管理和系统监控等功能。

## 🚀 项目特性

### 核心功能
- **文件管理**: 支持多种文件类型的上传、下载、在线预览
- **用户系统**: 游客、注册用户、管理员三级权限管理
- **在线预览**: 支持视频播放、文档阅读、图片查看
- **系统监控**: 实时监控CPU、内存、磁盘、进程状态
- **安全保护**: 文件类型检查、路径安全验证、权限控制

### 技术栈
- **后端**: C++17, SQLite3, HTTP服务器
- **前端**: Vue3, 现代化响应式UI
- **数据库**: SQLite3
- **构建工具**: CMake

## 📁 项目结构

```
g00j_file_share/
├── include/           # 头文件
│   ├── server.h      # HTTP服务器
│   ├── database.h    # 数据库管理
│   ├── file_manager.h # 文件管理
│   ├── json_helper.h # JSON处理
│   └── system_monitor.h # 系统监控
├── src/              # 源代码文件
│   ├── main.cpp      # 主程序入口
│   ├── server.cpp    # HTTP服务器实现
│   ├── database.cpp  # 数据库操作实现
│   ├── file_manager.cpp # 文件管理实现
│   ├── json_helper.cpp # JSON处理实现
│   └── system_monitor.cpp # 系统监控实现
├── static/           # 前端静态文件
│   ├── index.html    # 主页面
│   ├── css/          # 样式文件
│   └── js/           # JavaScript文件
├── shared/           # 共享文件存储
│   ├── videos/       # 视频文件
│   ├── documents/    # 文档文件
│   ├── images/       # 图片文件
│   └── others/       # 其他文件
├── bin/              # 可执行文件
├── build/            # CMake构建目录
├── CMakeLists.txt    # CMake配置
├── build.sh          # 构建脚本
├── start.sh          # 启动脚本
└── README.md         # 项目说明
```

## 🛠️ 快速开始

### 环境要求
- Linux 系统 (Ubuntu 18.04+, CentOS 7+)
- GCC 7+ (支持C++17)
- CMake 3.10+
- SQLite3 开发库
- pkg-config

### 安装依赖

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libsqlite3-dev pkg-config
```

**CentOS/RHEL:**
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake sqlite-devel pkgconfig
```

### 构建项目
```bash
# 克隆或下载项目到本地
cd g00j_file_share

# 构建项目
./build.sh
```

### 启动服务
```bash
# 启动服务器
./start.sh
```

### 访问系统
打开浏览器访问: http://localhost:8080

## 👥 默认账户

- **管理员**: admin / admin123
- **普通用户**: 需要注册

## 📝 API 文档

### 用户认证
- `POST /api/login` - 用户登录
- `POST /api/register` - 用户注册  
- `POST /api/logout` - 用户登出

### 文件管理
- `GET /api/files` - 获取文件列表
- `POST /api/upload` - 上传文件
- `GET /api/download/{id}` - 下载文件
- `GET /api/preview/{id}` - 预览文件

### 系统监控
- `GET /api/system/status` - 系统状态
- `GET /api/system/processes` - 进程列表 (管理员)

## 🔧 配置说明

### 服务器配置
- 默认端口: 8080
- 最大文件大小: 50MB
- 静态文件目录: `static/`
- 文件存储目录: `shared/`

### 数据库配置
- 数据库文件: `bin/g00j_share.db`
- 自动创建表结构
- 自动创建默认管理员账户

## 🚀 开发指南

### 编译
```bash
# 开发模式编译
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### 测试
```bash
# 启动服务器后进行功能测试
curl -X POST http://localhost:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
```

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📞 联系方式

如有问题，请提交 Issue 或联系项目维护者。

---

**注意**: 首次运行时会自动创建数据库和默认目录结构。 