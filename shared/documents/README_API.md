# g00j小站 API 文档

## 基础信息

- 基础URL: `http://localhost:8080`
- 认证方式: Session Cookie
- 数据格式: JSON

## 认证接口

### 用户登录
```
POST /api/login
Content-Type: application/json

{
    "username": "用户名",
    "password": "密码"
}
```

响应：
```json
{
    "success": true,
    "user": {
        "id": 1,
        "username": "admin",
        "role": "admin"
    }
}
```

### 用户注册
```
POST /api/register
Content-Type: application/json

{
    "username": "新用户名",
    "password": "密码"
}
```

### 获取当前用户
```
GET /api/user
```

## 文件管理接口

### 获取文件列表
```
GET /api/files
```

响应：
```json
{
    "files": [
        {
            "id": 1,
            "filename": "示例文件.txt",
            "path": "/shared/documents/example.txt",
            "category": "document",
            "size": 1024,
            "upload_time": "2024-01-01 12:00:00",
            "is_public": true
        }
    ]
}
```

### 文件上传
```
POST /api/upload
Content-Type: multipart/form-data

file: [文件内容]
```

### 删除文件
```
DELETE /api/files/{id}
```

## 系统监控接口（管理员）

### 获取系统信息
```
GET /api/system
```

响应：
```json
{
    "cpu_usage": 45.2,
    "memory_total": 8192000,
    "memory_used": 4096000,
    "memory_usage_percent": 50.0,
    "disk_total": 1000000000,
    "disk_used": 500000000,
    "disk_usage_percent": 50.0,
    "uptime": "2 天 12 小时 30 分钟",
    "process_count": 245
}
```

### 获取进程列表
```
GET /api/processes
```

### 终止进程
```
POST /api/processes/{pid}/kill
```

## 错误处理

所有API在发生错误时返回：
```json
{
    "error": "错误信息描述"
}
```

常见HTTP状态码：
- 200: 成功
- 400: 请求参数错误
- 401: 未认证
- 403: 权限不足
- 404: 资源不存在
- 500: 服务器内部错误

## 使用示例

### JavaScript 调用示例

```javascript
// 登录
async function login(username, password) {
    const response = await fetch('/api/login', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ username, password })
    });
    
    return await response.json();
}

// 获取文件列表
async function getFiles() {
    const response = await fetch('/api/files');
    return await response.json();
}

// 上传文件
async function uploadFile(file) {
    const formData = new FormData();
    formData.append('file', file);
    
    const response = await fetch('/api/upload', {
        method: 'POST',
        body: formData
    });
    
    return await response.json();
}
```

### curl 示例

```bash
# 登录
curl -X POST http://localhost:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'

# 获取文件列表
curl http://localhost:8080/api/files

# 上传文件
curl -X POST http://localhost:8080/api/upload \
  -F "file=@example.txt"
```

## 注意事项

1. 所有需要认证的接口都需要有效的Session Cookie
2. 文件上传有大小限制（默认50MB）
3. 只有白名单中的文件类型可以上传
4. 系统监控接口仅管理员可访问
5. 文件路径会进行安全检查，防止路径遍历攻击 