#!/bin/bash

# 设置服务名称
SERVICE_NAME="112_file_share"
PID_FILE="/var/run/${SERVICE_NAME}.pid"
LOG_FILE="/var/log/${SERVICE_NAME}.log"

# 检查是否为root用户
if [ "$EUID" -ne 0 ]; then 
    echo "请使用root权限运行此脚本"
    exit 1
fi

start() {
    if [ -f "$PID_FILE" ]; then
        echo "服务似乎已经在运行中。如果确认没有运行，请删除 ${PID_FILE}"
        exit 1
    fi
    
    echo "正在启动 ${SERVICE_NAME}..."
    cd /root/SHARE/SHARE
    nohup ./bin/112_file_share > "$LOG_FILE" 2>&1 &
    echo $! > "$PID_FILE"
    echo "服务已启动，PID: $(cat ${PID_FILE})"
    echo "日志文件位置: ${LOG_FILE}"
}

stop() {
    if [ ! -f "$PID_FILE" ]; then
        echo "PID文件不存在，服务可能没有运行"
        return
    fi
    
    PID=$(cat "$PID_FILE")
    echo "正在停止服务 (PID: $PID)..."
    kill $PID
    rm -f "$PID_FILE"
    echo "服务已停止"
}

status() {
    if [ -f "$PID_FILE" ]; then
        PID=$(cat "$PID_FILE")
        if ps -p $PID > /dev/null; then
            echo "服务正在运行 (PID: $PID)"
            echo "最近的日志:"
            tail -n 5 "$LOG_FILE"
        else
            echo "服务未运行，但PID文件存在。可能需要清理。"
            rm -f "$PID_FILE"
        fi
    else
        echo "服务未运行"
    fi
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        stop
        sleep 2
        start
        ;;
    status)
        status
        ;;
    *)
        echo "用法: $0 {start|stop|restart|status}"
        exit 1
        ;;
esac

exit 0