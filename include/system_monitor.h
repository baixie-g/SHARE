#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

// 系统资源信息结构
struct SystemInfo {
    double cpu_usage;        // CPU使用率 (%)
    double memory_usage;     // 内存使用率 (%)
    long total_memory;       // 总内存 (KB)
    long used_memory;        // 已用内存 (KB)
    long free_memory;        // 空闲内存 (KB)
    double disk_usage;       // 磁盘使用率 (%)
    long total_disk;         // 总磁盘空间 (KB)
    long used_disk;          // 已用磁盘空间 (KB)
    long free_disk;          // 空闲磁盘空间 (KB)
    std::string uptime;      // 系统运行时间
    int process_count;       // 进程总数
    double load_average_1;   // 1分钟平均负载
    double load_average_5;   // 5分钟平均负载
    double load_average_15;  // 15分钟平均负载
};

// 进程信息结构
struct ProcessInfo {
    int pid;                    // 进程ID
    std::string name;           // 进程名称
    std::string user;           // 进程所有者
    std::string state;          // 进程状态
    double cpu_percent;         // CPU使用率
    double memory_percent;      // 内存使用率
    long memory_usage;          // 内存使用量 (KB)
    std::string start_time;     // 启动时间
    std::string command;        // 完整命令行
};

// 网络接口信息结构
struct NetworkInterface {
    std::string name;           // 接口名称
    std::string ip_address;     // IP地址
    std::string mac_address;    // MAC地址
    long bytes_sent;            // 发送字节数
    long bytes_received;        // 接收字节数
    long packets_sent;          // 发送包数
    long packets_received;      // 接收包数
    bool is_up;                 // 接口状态
};

/**
 * 系统监控类
 * 提供CPU、内存、磁盘、进程、网络等系统信息的监控功能
 */
class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();

    // === 系统信息 ===
    // 获取系统整体信息
    SystemInfo getSystemInfo();
    
    // 获取CPU使用率
    double getCpuUsage();
    
    // 获取内存使用情况
    std::map<std::string, long> getMemoryInfo();
    
    // 获取磁盘使用情况
    std::map<std::string, long> getDiskInfo(const std::string& path = "/");
    
    // 获取系统运行时间
    std::string getUptime();
    
    // 获取系统负载
    std::vector<double> getLoadAverage();

    // === 进程管理 ===
    // 获取所有进程列表
    std::vector<ProcessInfo> getAllProcesses();
    
    // 获取指定用户的进程
    std::vector<ProcessInfo> getUserProcesses(const std::string& username);
    
    // 根据进程名搜索进程
    std::vector<ProcessInfo> findProcessesByName(const std::string& name);
    
    // 获取进程详细信息
    ProcessInfo getProcessInfo(int pid);
    
    // 终止进程
    bool killProcess(int pid, int signal = 15);  // 默认使用SIGTERM
    
    // 检查进程是否存在
    bool isProcessRunning(int pid);

    // === 网络监控 ===
    // 获取网络接口信息
    std::vector<NetworkInterface> getNetworkInterfaces();
    
    // 获取网络统计信息
    std::map<std::string, long> getNetworkStats();
    
    // 检查端口是否被占用
    bool isPortInUse(int port);
    
    // 获取监听端口列表
    std::vector<int> getListeningPorts();

    // === 系统性能 ===
    // 获取IO统计信息
    std::map<std::string, long> getIOStats();
    
    // 获取温度信息 (如果可用)
    std::map<std::string, double> getTemperatures();
    
    // 获取系统启动时间
    std::string getBootTime();

    // === 监控配置 ===
    // 设置监控间隔
    void setMonitorInterval(int seconds) { monitor_interval_ = seconds; }
    
    // 启动持续监控
    void startContinuousMonitoring();
    
    // 停止持续监控
    void stopContinuousMonitoring();
    
    // 检查是否正在监控
    bool isMonitoring() const { return monitoring_; }

    // === 工具函数 ===
    // 格式化字节大小
    static std::string formatBytes(long bytes);
    
    // 格式化百分比
    static std::string formatPercentage(double percentage);
    
    // 格式化时间
    static std::string formatTime(long seconds);

    // 系统状态信息
    static std::map<std::string, std::string> get_system_status();
    
    // CPU信息
    static double get_cpu_usage();
    static std::string get_cpu_info();
    
    // 内存信息
    static std::unordered_map<std::string, long> get_memory_info();
    
    // 磁盘信息
    static std::unordered_map<std::string, long> get_disk_info(const std::string& path = "/");
    
    // 网络信息
    static std::map<std::string, std::string> get_network_info();
    static std::vector<std::unordered_map<std::string, std::string>> get_network_interfaces();
    
    // 进程信息
    static std::vector<std::map<std::string, std::string>> get_processes();
    static bool kill_process(int pid);
    
    // 系统负载
    static std::vector<double> get_load_average();
    
    // 系统运行时间
    static std::string get_uptime();

private:
    std::string proc_path;       // /proc路径
    int monitor_interval_;
    bool monitoring_;
    std::vector<double> cpu_samples_;
    
    // 读取文件内容
    std::string read_file(const std::string& filepath);
    
    // 读取文件的第一行
    std::string readFirstLine(const std::string& filepath);
    
    // 分割字符串
    std::vector<std::string> split_string(const std::string& str, char delimiter);
    
    // 去除字符串首尾空白
    std::string trim(const std::string& str);
    
    // 解析CPU使用率
    double parse_cpu_usage();
    
    // 解析内存信息
    void parse_memory_info(long& total, long& free);
    
    // 解析磁盘信息
    void parse_disk_info(const std::string& path, long& total, long& free);
    
    // 解析负载信息
    void parse_load_average(double load[3]);
    
    // 解析运行时间
    std::string parse_uptime();
    
    // 解析进程信息
    ProcessInfo parse_process_info(int pid);
    
    // 解析/proc/[pid]/status获取进程状态
    std::map<std::string, std::string> parseProcessStatus(int pid);
    
    // 解析/proc/net/dev获取网络接口信息
    std::vector<NetworkInterface> parseNetworkInterfaces();
    
    // 辅助方法
    bool process_exists(int pid);
    std::string format_memory_size(long size_kb);
    bool kill_process(int pid, int signal);
    int get_process_count();
    int get_thread_count();
    bool is_system_healthy();
    void set_proc_path(const std::string& path);
    
    // 获取当前时间戳
    long getCurrentTimestamp();
    
    // 计算时间差
    long getTimeDifference(long start_time);
    
    // 监控线程函数
    void monitoringThread();

    static std::string read_file_content(const std::string& filepath);
    static long parse_memory_value(const std::string& value);
}; 