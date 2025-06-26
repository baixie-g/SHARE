# g00j小站 文件共享系统 Makefile

.PHONY: all build clean start install help

# 默认目标
all: build

# 构建项目
build:
	@echo "开始构建 g00j小站..."
	@./build.sh

# 启动服务器
start:
	@echo "启动 g00j小站服务器..."
	@./start.sh

# 清理构建文件
clean:
	@echo "清理构建文件..."
	@rm -rf build/
	@rm -rf bin/
	@echo "清理完成"

# 安装依赖 (Ubuntu/Debian)
install:
	@echo "安装系统依赖..."
	@sudo apt update
	@sudo apt install -y cmake g++ pkg-config libsqlite3-dev libssl-dev
	@echo "依赖安装完成"

# 显示帮助信息
help:
	@echo "g00j小站 文件共享系统"
	@echo ""
	@echo "可用命令:"
	@echo "  make build    - 构建项目"
	@echo "  make start    - 启动服务器"
	@echo "  make clean    - 清理构建文件"
	@echo "  make install  - 安装系统依赖"
	@echo "  make help     - 显示此帮助信息"
	@echo ""
	@echo "快速开始:"
	@echo "  1. make install  # 安装依赖"
	@echo "  2. make build    # 构建项目"
	@echo "  3. make start    # 启动服务器"
	@echo ""
	@echo "访问地址: http://localhost:8080"
	@echo "默认管理员: admin / admin123" 