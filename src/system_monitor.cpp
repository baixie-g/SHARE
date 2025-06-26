#include "system_monitor.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <dirent.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <algorithm>
#include <ctime>

SystemMonitor::SystemMonitor() {
    cpu_history_.reserve(MAX_HISTORY_SIZE);
    memory_history_.reserve(MAX_HISTORY_SIZE);
}

SystemInfo SystemMonitor::get_system_info() {
    SystemInfo info;
    
    info.cpu_usage = get_cpu_usage();
    
    auto memory_info = get_memory_info();
    info.total_memory_kb = memory_info["total"];
    info.free_memory_kb = memory_info["free"];
    info.used_memory_kb = info.total_memory_kb - info.free_memory_kb;
    info.memory_usage_percent = (double)info.used_memory_kb / info.total_memory_kb * 100.0;
    
    auto disk_info = get_disk_info();
    info.total_disk_kb = disk_info["total"];
    info.free_disk_kb = disk_info["free"];
    info.used_disk_kb = info.total_disk_kb - info.free_disk_kb;
    info.disk_usage_percent = (double)info.used_disk_kb / info.total_disk_kb * 100.0;
    
    info.uptime = get_system_uptime();
    info.process_count = get_process_list().size();
    
    // 获取负载平均值
    std::string loadavg_content = read_file("/proc/loadavg");
    if (!loadavg_content.empty()) {
        std::istringstream iss(loadavg_content);
        iss >> info.load_average[0] >> info.load_average[1] >> info.load_average[2];
    }
    
    return info;
}

double SystemMonitor::get_cpu_usage() {
    static long last_total = 0, last_idle = 0;
    
    std::string stat_content = read_file("/proc/stat");
    if (stat_content.empty()) {
        return 0.0;
    }
    
    std::istringstream iss(stat_content);
    std::string cpu_label;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    
    iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long total_diff = total - last_total;
    long idle_diff = idle - last_idle;
    
    double cpu_usage = 0.0;
    if (total_diff > 0) {
        cpu_usage = (double)(total_diff - idle_diff) / total_diff * 100.0;
    }
    
    last_total = total;
    last_idle = idle;
    
    return cpu_usage;
}

std::unordered_map<std::string, long> SystemMonitor::get_memory_info() {
    std::unordered_map<std::string, long> memory_info;
    
    std::string meminfo_content = read_file("/proc/meminfo");
    if (meminfo_content.empty()) {
        return memory_info;
    }
    
    std::istringstream iss(meminfo_content);
    std::string line;
    
    while (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        std::string key;
        long value;
        std::string unit;
        
        if (line_stream >> key >> value >> unit) {
            // 移除冒号
            if (!key.empty() && key.back() == ':') {
                key.pop_back();
            }
            
            if (key == "MemTotal") {
                memory_info["total"] = value;
            } else if (key == "MemFree") {
                memory_info["free"] = value;
            } else if (key == "MemAvailable") {
                memory_info["available"] = value;
            } else if (key == "Buffers") {
                memory_info["buffers"] = value;
            } else if (key == "Cached") {
                memory_info["cached"] = value;
            }
        }
    }
    
    return memory_info;
}

std::unordered_map<std::string, long> SystemMonitor::get_disk_info(const std::string& path) {
    std::unordered_map<std::string, long> disk_info;
    
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) == 0) {
        long block_size = stat.f_frsize;
        disk_info["total"] = (stat.f_blocks * block_size) / 1024; // KB
        disk_info["free"] = (stat.f_bavail * block_size) / 1024;   // KB
        disk_info["used"] = disk_info["total"] - disk_info["free"];
    }
    
    return disk_info;
}

std::string SystemMonitor::get_system_uptime() {
    std::string uptime_content = read_file("/proc/uptime");
    if (uptime_content.empty()) {
        return "Unknown";
    }
    
    double uptime_seconds = parse_double(uptime_content.substr(0, uptime_content.find(' ')));
    
    int days = uptime_seconds / 86400;
    int hours = (uptime_seconds - days * 86400) / 3600;
    int minutes = (uptime_seconds - days * 86400 - hours * 3600) / 60;
    
    std::ostringstream oss;
    if (days > 0) {
        oss << days << " 天 ";
    }
    if (hours > 0) {
        oss << hours << " 小时 ";
    }
    oss << minutes << " 分钟";
    
    return oss.str();
}

std::vector<ProcessInfo> SystemMonitor::get_process_list() {
    std::vector<ProcessInfo> processes;
    
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return processes;
    }
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (!isdigit(entry->d_name[0])) {
            continue; // 不是进程目录
        }
        
        int pid = std::stoi(entry->d_name);
        ProcessInfo proc_info = get_process_info(pid);
        
        if (proc_info.pid > 0) {
            processes.push_back(proc_info);
        }
    }
    
    closedir(proc_dir);
    
    // 按CPU使用率排序
    std::sort(processes.begin(), processes.end(), 
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.cpu_percent > b.cpu_percent;
              });
    
    return processes;
}

ProcessInfo SystemMonitor::get_process_info(int pid) {
    ProcessInfo info;
    info.pid = 0; // 表示获取失败
    
    // 读取进程状态
    std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
    std::string stat_content = read_file(stat_path);
    if (stat_content.empty()) {
        return info;
    }
    
    std::istringstream iss(stat_content);
    std::string token;
    std::vector<std::string> tokens;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.size() < 24) {
        return info;
    }
    
    info.pid = pid;
    info.name = tokens[1];
    
    // 移除进程名的括号
    if (info.name.front() == '(' && info.name.back() == ')') {
        info.name = info.name.substr(1, info.name.length() - 2);
    }
    
    info.status = tokens[2];
    
    // 获取内存使用（RSS * 页面大小 / 1024 = KB）
    long rss_pages = parse_long(tokens[23]);
    long page_size = sysconf(_SC_PAGESIZE);
    info.memory_kb = (rss_pages * page_size) / 1024;
    
    // 获取CPU时间
    long utime = parse_long(tokens[13]);
    long stime = parse_long(tokens[14]);
    // long total_time = utime + stime; // 暂时不使用
    
    // 简单的CPU使用率计算（需要时间间隔来计算准确值）
    info.cpu_percent = 0.0; // 这里可以实现更复杂的CPU使用率计算
    
    // 避免未使用变量警告
    (void)utime;
    (void)stime;
    
    // 获取进程用户
    std::string status_path = "/proc/" + std::to_string(pid) + "/status";
    std::string status_content = read_file(status_path);
    
    std::istringstream status_stream(status_content);
    std::string line;
    while (std::getline(status_stream, line)) {
        if (line.find("Uid:") == 0) {
            std::istringstream uid_stream(line);
            std::string uid_label;
            int uid;
            uid_stream >> uid_label >> uid;
            
            // 简化处理，实际应该查询用户名
            info.user = std::to_string(uid);
            break;
        }
    }
    
    // 获取命令行
    std::string cmdline_path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::string cmdline_content = read_file(cmdline_path);
    if (!cmdline_content.empty()) {
        // 替换空字符为空格
        std::replace(cmdline_content.begin(), cmdline_content.end(), '\0', ' ');
        info.command = cmdline_content;
    } else {
        info.command = "[" + info.name + "]";
    }
    
    return info;
}

bool SystemMonitor::kill_process(int pid, int signal) {
    return kill(pid, signal) == 0;
}

bool SystemMonitor::is_process_running(int pid) {
    return kill(pid, 0) == 0;
}

std::vector<NetworkInfo> SystemMonitor::get_network_interfaces() {
    std::vector<NetworkInfo> interfaces;
    
    std::string net_dev_content = read_file("/proc/net/dev");
    if (net_dev_content.empty()) {
        return interfaces;
    }
    
    std::istringstream iss(net_dev_content);
    std::string line;
    
    // 跳过头部两行
    std::getline(iss, line);
    std::getline(iss, line);
    
    while (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        std::string interface_name;
        
        if (std::getline(line_stream, interface_name, ':')) {
            interface_name = trim(interface_name);
            
            NetworkInfo info;
            info.interface = interface_name;
            info.is_up = true; // 简化处理
            
            // 读取统计数据
            line_stream >> info.bytes_received >> info.packets_received;
            
            // 跳过一些字段到发送数据
            for (int i = 0; i < 6; i++) {
                long dummy;
                line_stream >> dummy;
            }
            
            line_stream >> info.bytes_sent >> info.packets_sent;
            
            interfaces.push_back(info);
        }
    }
    
    return interfaces;
}

void SystemMonitor::record_system_stats() {
    auto cpu_usage = get_cpu_usage();
    auto memory_info = get_memory_info();
    
    std::unordered_map<std::string, double> cpu_data;
    cpu_data["usage"] = cpu_usage;
    cpu_data["timestamp"] = std::time(nullptr);
    
    std::unordered_map<std::string, long> memory_data;
    memory_data["total"] = memory_info["total"];
    memory_data["used"] = memory_info["total"] - memory_info["free"];
    memory_data["timestamp"] = std::time(nullptr);
    
    cpu_history_.push_back(cpu_data);
    memory_history_.push_back(memory_data);
    
    // 保持历史数据大小
    if (cpu_history_.size() > MAX_HISTORY_SIZE) {
        cpu_history_.erase(cpu_history_.begin());
    }
    if (memory_history_.size() > MAX_HISTORY_SIZE) {
        memory_history_.erase(memory_history_.begin());
    }
}

std::vector<std::unordered_map<std::string, double>> SystemMonitor::get_cpu_history(int minutes) {
    std::vector<std::unordered_map<std::string, double>> result;
    
    int count = std::min(minutes, (int)cpu_history_.size());
    if (count > 0) {
        result.assign(cpu_history_.end() - count, cpu_history_.end());
    }
    
    return result;
}

std::vector<std::unordered_map<std::string, long>> SystemMonitor::get_memory_history(int minutes) {
    std::vector<std::unordered_map<std::string, long>> result;
    
    int count = std::min(minutes, (int)memory_history_.size());
    if (count > 0) {
        result.assign(memory_history_.end() - count, memory_history_.end());
    }
    
    return result;
}

// 辅助方法实现
std::string SystemMonitor::read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

std::vector<std::string> SystemMonitor::split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    
    while (std::getline(iss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

double SystemMonitor::parse_double(const std::string& str) {
    try {
        return std::stod(str);
    } catch (const std::exception&) {
        return 0.0;
    }
}

long SystemMonitor::parse_long(const std::string& str) {
    try {
        return std::stol(str);
    } catch (const std::exception&) {
        return 0;
    }
}

std::string SystemMonitor::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
} 