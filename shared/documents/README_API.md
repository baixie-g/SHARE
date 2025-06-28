# 112小站 API 文档

## 简介

112小站是一个轻量级的文件共享系统，提供以下主要功能：

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

欢迎使用 112小站！
