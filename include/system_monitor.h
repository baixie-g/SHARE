#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// 进程信息结构
struct ProcessInfo {
    int pid;
    std::string name;
    std::string user;
    double cpu_percent;
    long memory_kb;
    std::string status;
    std::string command;
};

// 系统资源信息结构
struct SystemInfo {
    double cpu_usage;
    long total_memory_kb;
    long free_memory_kb;
    long used_memory_kb;
    double memory_usage_percent;
    
    long total_disk_kb;
    long free_disk_kb;
    long used_disk_kb;
    double disk_usage_percent;
    
    std::string uptime;
    int process_count;
    double load_average[3]; // 1, 5, 15分钟平均负载
};

// 网络信息结构
struct NetworkInfo {
    std::string interface;
    long bytes_sent;
    long bytes_received;
    long packets_sent;
    long packets_received;
    bool is_up;
};

// 系统监控类
class SystemMonitor {
public:
    SystemMonitor();
    
    // 系统资源监控
    SystemInfo get_system_info();
    double get_cpu_usage();
    std::unordered_map<std::string, long> get_memory_info();
    std::unordered_map<std::string, long> get_disk_info(const std::string& path = "/");
    std::string get_system_uptime();
    
    // 进程监控
    std::vector<ProcessInfo> get_process_list();
    ProcessInfo get_process_info(int pid);
    bool kill_process(int pid, int signal = 15); // SIGTERM
    bool is_process_running(int pid);
    
    // 网络监控
    std::vector<NetworkInfo> get_network_interfaces();
    std::unordered_map<std::string, std::string> get_network_stats();
    
    // 文件系统监控
    std::vector<std::string> get_mounted_filesystems();
    std::unordered_map<std::string, long> get_filesystem_usage(const std::string& path);
    
    // 历史数据（简单实现）
    void record_system_stats();
    std::vector<std::unordered_map<std::string, double>> get_cpu_history(int minutes = 60);
    std::vector<std::unordered_map<std::string, long>> get_memory_history(int minutes = 60);
    
private:
    // 缓存数据
    std::vector<std::unordered_map<std::string, double>> cpu_history_;
    std::vector<std::unordered_map<std::string, long>> memory_history_;
    static const int MAX_HISTORY_SIZE = 1440; // 24小时的分钟数
    
    // 辅助方法
    std::string read_file(const std::string& filepath);
    std::vector<std::string> split_string(const std::string& str, char delimiter);
    double parse_double(const std::string& str);
    long parse_long(const std::string& str);
    std::string trim(const std::string& str);
}; 