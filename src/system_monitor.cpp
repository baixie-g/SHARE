#include "system_monitor.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <dirent.h>
#include <sys/statvfs.h>
#include <signal.h>
#include <algorithm>
#include <iomanip>

SystemMonitor::SystemMonitor() : proc_path("/proc") {
}

SystemMonitor::~SystemMonitor() {
}

SystemInfo SystemMonitor::getSystemInfo() {
    SystemInfo info;
    
    info.cpu_usage = getCpuUsage();
    
    auto memInfo = getMemoryInfo();
    info.total_memory = memInfo["total"];
    info.free_memory = memInfo["free"];
    info.used_memory = info.total_memory - info.free_memory;
    info.memory_usage = (double)info.used_memory / info.total_memory * 100.0;
    
    auto diskInfo = getDiskInfo("/");
    info.total_disk = diskInfo["total"];
    info.used_disk = diskInfo["used"];
    info.free_disk = diskInfo["free"];
    info.disk_usage = (double)info.used_disk / info.total_disk * 100.0;
    
    info.uptime = getUptime();
    
    auto loadAvg = getLoadAverage();
    if (loadAvg.size() >= 3) {
        info.load_average_1 = loadAvg[0];
        info.load_average_5 = loadAvg[1];
        info.load_average_15 = loadAvg[2];
    }
    
    info.process_count = getAllProcesses().size();
    
    return info;
}

double SystemMonitor::getCpuUsage() {
    return parse_cpu_usage();
}

std::map<std::string, long> SystemMonitor::getMemoryInfo() {
    long total, free;
    parse_memory_info(total, free);
    
    std::map<std::string, long> mem_info;
    mem_info["total"] = total;
    mem_info["free"] = free;
    mem_info["used"] = total - free;
    
    return mem_info;
}

std::map<std::string, long> SystemMonitor::getDiskInfo(const std::string& path) {
    long total, free;
    parse_disk_info(path, total, free);
    
    std::map<std::string, long> info;
    info["total"] = total;
    info["free"] = free;
    info["used"] = total - free;
    
    return info;
}

std::string SystemMonitor::getUptime() {
    return parse_uptime();
}

std::vector<double> SystemMonitor::getLoadAverage() {
    double load[3];
    parse_load_average(load);
    return {load[0], load[1], load[2]};
}

std::vector<ProcessInfo> SystemMonitor::getAllProcesses() {
    std::vector<ProcessInfo> processes;
    
    DIR* proc_dir = opendir(proc_path.c_str());
    if (!proc_dir) {
        return processes;
    }
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr && processes.size() < 10000) {
        // 检查是否为数字目录（进程ID）
        std::string name = entry->d_name;
        if (std::all_of(name.begin(), name.end(), ::isdigit)) {
            int pid = std::stoi(name);
            ProcessInfo info = parse_process_info(pid);
            if (info.pid > 0) {
                processes.push_back(info);
            }
        }
    }
    
    closedir(proc_dir);
    
    // 按PID排序
    std::sort(processes.begin(), processes.end(), 
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.pid < b.pid;
              });
    
    return processes;
}

std::string SystemMonitor::read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

double SystemMonitor::parse_cpu_usage() {
    std::string stat_content = read_file(proc_path + "/stat");
    if (stat_content.empty()) {
        return 0.0;
    }
    
    std::istringstream iss(stat_content);
    std::string line;
    std::getline(iss, line);
    
    // 解析第一行 (总CPU统计)
    std::istringstream line_iss(line);
    std::string cpu_label;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    
    line_iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long active = total - idle - iowait;
    
    if (total == 0) {
        return 0.0;
    }
    
    return (static_cast<double>(active) / static_cast<double>(total)) * 100.0;
}

void SystemMonitor::parse_memory_info(long& total, long& free) {
    std::string meminfo_content = read_file(proc_path + "/meminfo");
    if (meminfo_content.empty()) {
        total = free = 0;
        return;
    }
    
    std::istringstream iss(meminfo_content);
    std::string line;
    
    total = free = 0;
    long available = 0;
    
    while (std::getline(iss, line)) {
        std::istringstream line_iss(line);
        std::string key;
        long value;
        std::string unit;
        
        line_iss >> key >> value >> unit;
        
        if (key == "MemTotal:") {
            total = value;
        } else if (key == "MemAvailable:") {
            available = value;
        } else if (key == "MemFree:" && available == 0) {
            // 如果没有MemAvailable，使用MemFree
            free = value;
        }
    }
    
    if (available > 0) {
        free = available;
    }
}

void SystemMonitor::parse_disk_info(const std::string& path, long& total, long& free) {
    struct statvfs stat;
    
    if (statvfs(path.c_str(), &stat) != 0) {
        total = free = 0;
        return;
    }
    
    total = (stat.f_blocks * stat.f_frsize) / 1024; // KB
    free = (stat.f_bavail * stat.f_frsize) / 1024;  // KB
}

void SystemMonitor::parse_load_average(double load[3]) {
    std::string loadavg_content = read_file(proc_path + "/loadavg");
    if (loadavg_content.empty()) {
        load[0] = load[1] = load[2] = 0.0;
        return;
    }
    
    std::istringstream iss(loadavg_content);
    iss >> load[0] >> load[1] >> load[2];
}

std::string SystemMonitor::parse_uptime() {
    std::string uptime_content = read_file(proc_path + "/uptime");
    if (uptime_content.empty()) {
        return "0";
    }
    
    std::istringstream iss(uptime_content);
    double uptime_seconds;
    iss >> uptime_seconds;
    
    long total_seconds = static_cast<long>(uptime_seconds);
    long days = total_seconds / 86400;
    long hours = (total_seconds % 86400) / 3600;
    long minutes = (total_seconds % 3600) / 60;
    
    std::ostringstream oss;
    if (days > 0) {
        oss << days << "天 ";
    }
    oss << hours << "小时 " << minutes << "分钟";
    
    return oss.str();
}

ProcessInfo SystemMonitor::parse_process_info(int pid) {
    ProcessInfo info;
    info.pid = pid;
    
    std::string stat_path = proc_path + "/" + std::to_string(pid) + "/stat";
    std::string stat_content = read_file(stat_path);
    
    if (stat_content.empty()) {
        return info;
    }
    
    std::istringstream iss(stat_content);
    std::string temp;
    
    // 解析/proc/[pid]/stat格式
    iss >> temp; // pid
    iss >> info.name; // comm
    iss >> info.state; // state
    
    // 移除进程名的括号
    if (info.name.front() == '(' && info.name.back() == ')') {
        info.name = info.name.substr(1, info.name.length() - 2);
    }
    
    // 简化实现，设置默认值
    info.user = "unknown";
    info.cpu_percent = 0.0;
    info.memory_percent = 0.0;
    info.memory_usage = 0;
    info.start_time = "unknown";
    
    return info;
}

bool SystemMonitor::process_exists(int pid) {
    std::string proc_dir = proc_path + "/" + std::to_string(pid);
    std::ifstream file(proc_dir + "/stat");
    return file.good();
}

std::string SystemMonitor::format_memory_size(long size_kb) {
    const char* units[] = {"KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size_double = static_cast<double>(size_kb);
    
    while (size_double >= 1024.0 && unit_index < 3) {
        size_double /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size_double << " " << units[unit_index];
    return oss.str();
}

bool SystemMonitor::kill_process(int pid, int signal) {
    return kill(pid, signal) == 0;
}

int SystemMonitor::get_process_count() {
    std::vector<ProcessInfo> processes = getAllProcesses();
    return processes.size();
}

int SystemMonitor::get_thread_count() {
    // 简化实现，返回进程数的近似值
    return get_process_count() * 2;
}

std::vector<double> SystemMonitor::get_load_average() {
    std::vector<double> load_avg;
    
    std::string loadavg_content = read_file_content("/proc/loadavg");
    if (loadavg_content.empty()) {
        return load_avg;
    }
    
    std::istringstream ss(loadavg_content);
    double load1, load5, load15;
    ss >> load1 >> load5 >> load15;
    
    load_avg.push_back(load1);
    load_avg.push_back(load5);
    load_avg.push_back(load15);
    
    return load_avg;
}

std::string SystemMonitor::get_uptime() {
    std::string uptime_content = read_file_content("/proc/uptime");
    if (uptime_content.empty()) {
        return "Unknown";
    }
    
    std::istringstream ss(uptime_content);
    double uptime_seconds;
    ss >> uptime_seconds;
    
    int days = uptime_seconds / 86400;
    int hours = (uptime_seconds - days * 86400) / 3600;
    int minutes = (uptime_seconds - days * 86400 - hours * 3600) / 60;
    
    std::ostringstream result;
    if (days > 0) {
        result << days << " days ";
    }
    result << hours << " hours " << minutes << " minutes";
    
    return result.str();
}

std::unordered_map<std::string, long> SystemMonitor::get_memory_info() {
    std::unordered_map<std::string, long> memory_info;
    
    std::string meminfo_content = read_file_content("/proc/meminfo");
    if (meminfo_content.empty()) {
        return memory_info;
    }
    
    std::istringstream ss(meminfo_content);
    std::string line;
    
    while (std::getline(ss, line)) {
        if (line.find("MemTotal:") == 0) {
            memory_info["total"] = parse_memory_value(line);
        } else if (line.find("MemAvailable:") == 0) {
            memory_info["available"] = parse_memory_value(line);
        } else if (line.find("MemFree:") == 0) {
            memory_info["free"] = parse_memory_value(line);
        } else if (line.find("Buffers:") == 0) {
            memory_info["buffers"] = parse_memory_value(line);
        } else if (line.find("Cached:") == 0) {
            memory_info["cached"] = parse_memory_value(line);
        }
    }
    
    // 如果没有MemAvailable，则计算
    if (memory_info.find("available") == memory_info.end()) {
        memory_info["available"] = memory_info["free"] + memory_info["buffers"] + memory_info["cached"];
    }
    
    return memory_info;
}

std::unordered_map<std::string, long> SystemMonitor::get_disk_info(const std::string& path) {
    std::unordered_map<std::string, long> disk_info;
    
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) == 0) {
        disk_info["total"] = stat.f_blocks * stat.f_frsize;
        disk_info["free"] = stat.f_bavail * stat.f_frsize;
        disk_info["used"] = disk_info["total"] - disk_info["free"];
    }
    
    return disk_info;
}

bool SystemMonitor::is_system_healthy() {
    SystemInfo info = getSystemInfo();
    
    // 简单的健康检查：CPU、内存、磁盘使用率都不超过90%
    return info.cpu_usage < 90.0 && 
           info.memory_usage < 90.0 && 
           info.disk_usage < 90.0;
}

std::vector<std::unordered_map<std::string, std::string>> SystemMonitor::get_network_interfaces() {
    // 简化实现，返回空列表
    // 实际实现需要解析/proc/net/dev等文件
    return {};
}

void SystemMonitor::set_proc_path(const std::string& path) {
    proc_path = path;
}

std::map<std::string, std::string> SystemMonitor::get_system_status() {
    std::map<std::string, std::string> status;
    
    // CPU使用率
    double cpu_usage = get_cpu_usage();
    status["cpu_usage"] = std::to_string(cpu_usage);
    status["cpu_info"] = get_cpu_info();
    
    // 内存信息
    auto memory_info = get_memory_info();
    if (memory_info["total"] > 0) {
        double memory_usage = (double)(memory_info["total"] - memory_info["available"]) / memory_info["total"] * 100.0;
        status["memory_usage"] = std::to_string(memory_usage);
        status["memory_total"] = std::to_string(memory_info["total"]);
        status["memory_available"] = std::to_string(memory_info["available"]);
    }
    
    // 磁盘信息
    auto disk_info = get_disk_info();
    if (disk_info["total"] > 0) {
        double disk_usage = (double)(disk_info["total"] - disk_info["free"]) / disk_info["total"] * 100.0;
        status["disk_usage"] = std::to_string(disk_usage);
        status["disk_total"] = std::to_string(disk_info["total"]);
        status["disk_free"] = std::to_string(disk_info["free"]);
    }
    
    // 系统负载
    auto load_avg = get_load_average();
    if (load_avg.size() >= 3) {
        status["load_1min"] = std::to_string(load_avg[0]);
        status["load_5min"] = std::to_string(load_avg[1]);
        status["load_15min"] = std::to_string(load_avg[2]);
    }
    
    // 运行时间
    status["uptime"] = get_uptime();
    
    return status;
}

double SystemMonitor::get_cpu_usage() {
    static long prev_idle = 0, prev_total = 0;
    
    std::string stat_content = read_file_content("/proc/stat");
    if (stat_content.empty()) {
        return 0.0;
    }
    
    std::istringstream ss(stat_content);
    std::string line;
    std::getline(ss, line);
    
    std::istringstream cpu_ss(line);
    std::string cpu_label;
    cpu_ss >> cpu_label;
    
    long user, nice, system, idle, iowait, irq, softirq, steal;
    cpu_ss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long idle_time = idle + iowait;
    
    if (prev_total == 0) {
        prev_idle = idle_time;
        prev_total = total;
        return 0.0;
    }
    
    long total_diff = total - prev_total;
    long idle_diff = idle_time - prev_idle;
    
    double cpu_usage = 100.0 * (total_diff - idle_diff) / total_diff;
    
    prev_idle = idle_time;
    prev_total = total;
    
    return cpu_usage;
}

std::string SystemMonitor::get_cpu_info() {
    std::string cpuinfo_content = read_file_content("/proc/cpuinfo");
    if (cpuinfo_content.empty()) {
        return "Unknown";
    }
    
    std::istringstream ss(cpuinfo_content);
    std::string line;
    
    while (std::getline(ss, line)) {
        if (line.find("model name") == 0) {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string model = line.substr(colon_pos + 1);
                // 去除前导空格
                model.erase(model.begin(), std::find_if(model.begin(), model.end(),
                    [](unsigned char ch) { return !std::isspace(ch); }));
                return model;
            }
        }
    }
    
    return "Unknown";
}



std::map<std::string, std::string> SystemMonitor::get_network_info() {
    std::map<std::string, std::string> network_info;
    
    // 简化的网络信息获取
    network_info["status"] = "active";
    
    return network_info;
}

std::vector<std::map<std::string, std::string>> SystemMonitor::get_processes() {
    std::vector<std::map<std::string, std::string>> processes;
    
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return processes;
    }
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        // 检查是否是数字目录（进程ID）
        std::string name = entry->d_name;
        if (name.find_first_not_of("0123456789") != std::string::npos) {
            continue;
        }
        
        int pid = std::stoi(name);
        std::string stat_path = "/proc/" + name + "/stat";
        std::string comm_path = "/proc/" + name + "/comm";
        
        std::string stat_content = read_file_content(stat_path);
        std::string comm_content = read_file_content(comm_path);
        
        if (stat_content.empty() || comm_content.empty()) {
            continue;
        }
        
        // 移除进程名末尾的换行符
        if (!comm_content.empty() && comm_content.back() == '\n') {
            comm_content.pop_back();
        }
        
        std::map<std::string, std::string> process;
        process["pid"] = std::to_string(pid);
        process["name"] = comm_content;
        process["status"] = "R"; // 简化处理
        process["cpu"] = "0.0";
        process["memory"] = "0.0";
        
        processes.push_back(process);
        
        // 限制返回的进程数量
        if (processes.size() >= 50) {
            break;
        }
    }
    
    closedir(proc_dir);
    return processes;
}

bool SystemMonitor::kill_process(int pid) {
    return kill(pid, SIGTERM) == 0;
}



std::string SystemMonitor::read_file_content(const std::string& filepath) {
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
    std::istringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

long SystemMonitor::parse_memory_value(const std::string& line) {
    std::istringstream ss(line);
    std::string label;
    long value;
    
    ss >> label >> value;
    return value * 1024; // 转换为字节
} 