# 🚀 112 文件共享系统 - 服务器部署指南

## 📋 快速部署

### 1️⃣ 克隆项目
```bash
# 在服务器上执行
git clone git@github.com:baixie-g/SHARE.git
# 或使用HTTPS
git clone https://github.com/baixie-g/SHARE.git

cd SHARE
```

### 2️⃣ 一键部署
```bash
# 给脚本执行权限
chmod +x deploy.sh

# 运行部署脚本
./deploy.sh
```

### 3️⃣ 选择部署模式
部署脚本提供三种模式：

- **模式1**: 快速部署 (仅编译，手动启动)
- **模式2**: 完整部署 (自动安装依赖、配置systemd服务)  
- **模式3**: 生产环境部署 (完整部署 + Nginx反向代理 + 防火墙配置)

推荐生产环境使用模式3。

## 📋 手动部署步骤

### 1. 安装依赖

#### Ubuntu/Debian:
```bash
sudo apt update
sudo apt install -y cmake g++ pkg-config libsqlite3-dev libssl-dev git build-essential
```

#### CentOS/RHEL:
```bash
sudo yum groupinstall -y "Development Tools"
sudo yum install -y cmake gcc-c++ pkg-config sqlite-devel openssl-devel git
```

### 2. 编译项目
```bash
chmod +x build.sh start.sh
./build.sh
```

### 3. 配置systemd服务
```bash
sudo tee /etc/systemd/system/112-share.service > /dev/null <<EOF
[Unit]
Description=112 File Share Server
After=network.target

[Service]
Type=simple
User=$USER
WorkingDirectory=$(pwd)
ExecStart=$(pwd)/bin/112_file_share
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable 112-share
sudo systemctl start 112-share
```

### 4. 配置防火墙
```bash
# Ubuntu
sudo ufw allow 8080/tcp

# CentOS
sudo firewall-cmd --permanent --add-port=8080/tcp
sudo firewall-cmd --reload
```

## 🌐 Nginx 反向代理 (可选)

### 1. 安装Nginx
```bash
# Ubuntu/Debian
sudo apt install nginx

# CentOS/RHEL
sudo yum install nginx
```

### 2. 配置虚拟主机
```bash
sudo nano /etc/nginx/sites-available/112-share
```

添加配置：
```nginx
server {
    listen 80;
    server_name your-domain.com;  # 替换为你的域名

    client_max_body_size 100M;

    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

### 3. 启用配置
```bash
sudo ln -s /etc/nginx/sites-available/112-share /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl restart nginx
```

## 🔒 SSL证书配置

### 使用 Let's Encrypt
```bash
# 安装certbot
sudo apt install certbot python3-certbot-nginx

# 获取证书
sudo certbot --nginx -d your-domain.com

# 自动续期
echo "0 12 * * * /usr/bin/certbot renew --quiet" | sudo crontab -
```

## 🔧 服务管理命令

```bash
# 查看服务状态
sudo systemctl status 112-share

# 启动服务
sudo systemctl start 112-share

# 停止服务
sudo systemctl stop 112-share

# 重启服务
sudo systemctl restart 112-share

# 查看日志
sudo journalctl -u 112-share -f

# 查看最近100行日志
sudo journalctl -u 112-share -n 100
```

## 📊 系统要求

### 最低配置
- **CPU**: 1核
- **内存**: 512MB
- **存储**: 1GB可用空间
- **操作系统**: Linux (Ubuntu 18.04+, CentOS 7+, Debian 9+)

### 推荐配置
- **CPU**: 2核+
- **内存**: 2GB+
- **存储**: 10GB+可用空间
- **网络**: 100Mbps+

## 🛡️ 安全建议

1. **修改默认密码**: 部署完成后立即修改admin账户密码
2. **定期备份**: 备份数据库文件 `bin/112_share.db`
3. **更新系统**: 定期更新操作系统和依赖
4. **监控日志**: 定期检查系统和应用日志
5. **限制访问**: 使用防火墙限制不必要的端口访问

## 🔄 更新部署

```bash
# 停止服务
sudo systemctl stop 112-share

# 拉取最新代码
git pull origin master

# 重新编译
./build.sh

# 启动服务
sudo systemctl start 112-share
```

## 📞 问题排查

### 服务启动失败
```bash
# 查看详细错误信息
sudo journalctl -u 112-share --no-pager

# 检查端口占用
sudo netstat -tlnp | grep 8080

# 手动启动调试
cd /path/to/SHARE
./bin/112_file_share
```

### 编译错误
```bash
# 检查依赖是否完整安装
cmake --version
g++ --version
pkg-config --version

# 清理重新编译
rm -rf build bin obj
./build.sh
```

## 📞 技术支持

- **GitHub**: https://github.com/baixie-g/SHARE
- **默认管理员**: admin / admin123
- **默认端口**: 8080

---

**注意**: 生产环境部署时请务必修改默认密码！ 