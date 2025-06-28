#!/bin/bash

# ===================================================
# 112 文件共享系统 - 服务器部署脚本
# ===================================================

set -e  # 遇到错误立即退出

echo "=========================================="
echo "🚀 112 文件共享系统部署脚本"
echo "=========================================="

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 检测操作系统
detect_os() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        OS=$NAME
        VER=$VERSION_ID
    elif type lsb_release >/dev/null 2>&1; then
        OS=$(lsb_release -si)
        VER=$(lsb_release -sr)
    else
        OS=$(uname -s)
        VER=$(uname -r)
    fi
    echo -e "${BLUE}检测到操作系统: $OS $VER${NC}"
}

# 安装依赖
install_dependencies() {
    echo -e "${YELLOW}正在安装系统依赖...${NC}"
    
    if command -v apt &> /dev/null; then
        # Ubuntu/Debian
        sudo apt update
        sudo apt install -y cmake g++ pkg-config libsqlite3-dev libssl-dev git build-essential
        
        # 可选：安装nginx和supervisor用于生产环境
        read -p "是否安装Nginx和Supervisor? (y/n): " install_extras
        if [[ $install_extras == "y" || $install_extras == "Y" ]]; then
            sudo apt install -y nginx supervisor
        fi
        
    elif command -v yum &> /dev/null; then
        # CentOS/RHEL
        sudo yum groupinstall -y "Development Tools"
        sudo yum install -y cmake gcc-c++ pkg-config sqlite-devel openssl-devel git
        
        # 可选安装
        read -p "是否安装Nginx和Supervisor? (y/n): " install_extras
        if [[ $install_extras == "y" || $install_extras == "Y" ]]; then
            sudo yum install -y nginx supervisor
        fi
        
    elif command -v dnf &> /dev/null; then
        # Fedora
        sudo dnf groupinstall -y "Development Tools"
        sudo dnf install -y cmake gcc-c++ pkg-config sqlite-devel openssl-devel git
        
    else
        echo -e "${RED}❌ 不支持的操作系统，请手动安装依赖${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✅ 依赖安装完成${NC}"
}

# 编译项目
build_project() {
    echo -e "${YELLOW}正在编译项目...${NC}"
    
    # 确保脚本可执行
    chmod +x build.sh start.sh
    
    # 编译
    ./build.sh
    
    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}✅ 编译成功${NC}"
    else
        echo -e "${RED}❌ 编译失败${NC}"
        exit 1
    fi
}

# 配置防火墙
configure_firewall() {
    echo -e "${YELLOW}正在配置防火墙...${NC}"
    
    PORT=${1:-80}
    
    if command -v ufw &> /dev/null; then
        # Ubuntu防火墙
        sudo ufw allow $PORT/tcp
        echo -e "${GREEN}✅ UFW防火墙已开放端口 $PORT${NC}"
        
    elif command -v firewall-cmd &> /dev/null; then
        # CentOS防火墙
        sudo firewall-cmd --permanent --add-port=$PORT/tcp
        sudo firewall-cmd --reload
        echo -e "${GREEN}✅ Firewalld防火墙已开放端口 $PORT${NC}"
        
    else
        echo -e "${YELLOW}⚠️ 未检测到防火墙，请手动开放端口 $PORT${NC}"
    fi
}

# 创建systemd服务
create_systemd_service() {
    echo -e "${YELLOW}正在创建systemd服务...${NC}"
    
    CURRENT_DIR=$(pwd)
    SERVICE_USER=${1:-$USER}
    
    sudo tee /etc/systemd/system/112-share.service > /dev/null <<EOF
[Unit]
Description=112 File Share Server
After=network.target

[Service]
Type=simple
User=$SERVICE_USER
WorkingDirectory=$CURRENT_DIR
ExecStart=$CURRENT_DIR/bin/112_file_share
Restart=always
RestartSec=10
Environment=PATH=/usr/local/bin:/usr/bin:/bin

[Install]
WantedBy=multi-user.target
EOF

    sudo systemctl daemon-reload
    sudo systemctl enable 112-share
    
    echo -e "${GREEN}✅ Systemd服务创建完成${NC}"
}

# 配置Nginx反向代理
configure_nginx() {
    echo -e "${YELLOW}正在配置Nginx反向代理...${NC}"
    
    read -p "请输入域名或服务器IP地址: " server_name
    read -p "应用端口 (默认80): " app_port
    app_port=${app_port:-80}
    
    sudo tee /etc/nginx/sites-available/112-share > /dev/null <<EOF
server {
    listen 80;
    server_name $server_name;

    client_max_body_size 100M;

    location / {
        proxy_pass http://127.0.0.1:$app_port;
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
        
        # WebSocket支持
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
EOF

    # 启用站点
    sudo ln -sf /etc/nginx/sites-available/112-share /etc/nginx/sites-enabled/
    
    # 测试配置
    sudo nginx -t && sudo systemctl restart nginx
    
    echo -e "${GREEN}✅ Nginx配置完成${NC}"
}

# 启动服务
start_service() {
    echo -e "${YELLOW}正在启动服务...${NC}"
    
    sudo systemctl start 112-share
    sleep 2
    
    if sudo systemctl is-active --quiet 112-share; then
        echo -e "${GREEN}✅ 服务启动成功${NC}"
        
        # 获取服务器IP
        SERVER_IP=$(hostname -I | awk '{print $1}')
        
        echo -e "${GREEN}=========================================="
        echo -e "🎉 部署完成！"
        echo -e "=========================================="
        echo -e "📡 服务状态: $(sudo systemctl is-active 112-share)"
        echo -e "🌐 访问地址: http://$SERVER_IP:80"
        echo -e "👤 管理员账户: admin / admin123"
        echo -e "📝 修改密码: 登录后请及时修改默认密码"
        echo -e "🔧 服务管理:"
        echo -e "   启动: sudo systemctl start 112-share"
        echo -e "   停止: sudo systemctl stop 112-share"
        echo -e "   重启: sudo systemctl restart 112-share"
        echo -e "   状态: sudo systemctl status 112-share"
        echo -e "   日志: sudo journalctl -u 112-share -f"
        echo -e "==========================================${NC}"
    else
        echo -e "${RED}❌ 服务启动失败${NC}"
        echo -e "${YELLOW}查看日志: sudo journalctl -u 112-share${NC}"
        exit 1
    fi
}

# 主函数
main() {
    echo -e "${BLUE}开始部署流程...${NC}"
    
    # 检测系统
    detect_os
    
    # 询问部署选项
    echo -e "${YELLOW}请选择部署模式:${NC}"
    echo "1) 快速部署 (仅编译和启动)"
    echo "2) 完整部署 (安装依赖、编译、配置服务)"
    echo "3) 生产环境部署 (完整部署 + Nginx + 防火墙)"
    
    read -p "请输入选择 (1-3): " deploy_mode
    
    case $deploy_mode in
        1)
            build_project
            echo -e "${GREEN}✅ 快速部署完成，使用 ./start.sh 启动服务${NC}"
            ;;
        2)
            install_dependencies
            build_project
            create_systemd_service
            configure_firewall
            start_service
            ;;
        3)
            install_dependencies
            build_project
            create_systemd_service
            configure_firewall
            
            if command -v nginx &> /dev/null; then
                configure_nginx
            fi
            
            start_service
            ;;
        *)
            echo -e "${RED}❌ 无效选择${NC}"
            exit 1
            ;;
    esac
}

# 检查是否为root用户
if [[ $EUID -eq 0 ]]; then
   echo -e "${RED}❌ 不要使用root用户运行此脚本${NC}"
   echo -e "${YELLOW}建议创建普通用户：sudo adduser 112 && sudo usermod -aG sudo 112${NC}"
   exit 1
fi

# 运行主函数
main "$@" 