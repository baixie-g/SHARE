#include <iostream>
#include <signal.h>
#include <memory>
#include <chrono>
#include <thread>
#include <ctime>
#include "server.h"
#include "database.h"
#include "file_manager.h"
#include "system_monitor.h"
#include "json_helper.h"

// 全局变量，用于信号处理
std::unique_ptr<HttpServer> g_server;
std::unique_ptr<Database> g_database;
std::unique_ptr<FileManager> g_file_manager;
std::unique_ptr<SystemMonitor> g_system_monitor;

/**
 * 信号处理函数
 * 优雅地停止服务器
 */
void signalHandler(int signal) {
    std::cout << "\n收到停止信号 (" << signal << ")，正在关闭服务器..." << std::endl;
    
    if (g_server) {
        g_server->stop();
    }
    
    if (g_database) {
        g_database->close();
    }
    
    std::cout << "服务器已安全关闭。" << std::endl;
    exit(0);
}

/**
 * 注册API路由处理器
 */
void registerApiRoutes(HttpServer& server, Database& db, FileManager& fm, SystemMonitor& sm) {
    
    // === 用户认证相关API ===
    
    // 用户登录
    server.addRoute("POST", "/api/login", [&db](const HttpRequest& req) -> HttpResponse {
        HttpResponse response;
        
        try {
            auto json_data = JsonHelper::parseObject(req.body);
            
            if (!JsonHelper::hasRequiredFields(json_data, {"username", "password"})) {
                response.status_code = 400;
                response.body = JsonHelper::createErrorResponse("缺少用户名或密码");
                return response;
            }
            
            std::string username = JsonHelper::getString(json_data["username"]);
            std::string password = JsonHelper::getString(json_data["password"]);
            
            User* user = db.authenticateUser(username, password);
            if (user) {
                // 生成会话ID
                std::string session_id = "sess_" + std::to_string(time(nullptr)) + "_" + std::to_string(user->id);
                
                if (db.createSession(session_id, user->id)) {
                    response.headers["Set-Cookie"] = "session_id=" + session_id + "; Path=/; HttpOnly";
                    response.body = JsonHelper::createUserResponse(user->username, user->role, user->id);
                } else {
                    response.status_code = 500;
                    response.body = JsonHelper::createErrorResponse("创建会话失败");
                }
                
                delete user;
            } else {
                response.status_code = 401;
                response.body = JsonHelper::createErrorResponse("用户名或密码错误");
            }
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = JsonHelper::createErrorResponse("请求格式错误: " + std::string(e.what()));
        }
        
        response.headers["Content-Type"] = "application/json";
        return response;
    });
    
    // 用户注册
    server.addRoute("POST", "/api/register", [&db](const HttpRequest& req) -> HttpResponse {
        HttpResponse response;
        
        try {
            auto json_data = JsonHelper::parseObject(req.body);
            
            if (!JsonHelper::hasRequiredFields(json_data, {"username", "password"})) {
                response.status_code = 400;
                response.body = JsonHelper::createErrorResponse("缺少用户名或密码");
                return response;
            }
            
            std::string username = JsonHelper::getString(json_data["username"]);
            std::string password = JsonHelper::getString(json_data["password"]);
            
            // 检查用户名是否已存在
            User* existing_user = db.getUserByUsername(username);
            if (existing_user) {
                delete existing_user;
                response.status_code = 409;
                response.body = JsonHelper::createErrorResponse("用户名已存在");
                return response;
            }
            
            if (db.createUser(username, password, "user")) {
                response.body = JsonHelper::createSuccessResponse("用户注册成功");
            } else {
                response.status_code = 500;
                response.body = JsonHelper::createErrorResponse("用户注册失败");
            }
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = JsonHelper::createErrorResponse("请求格式错误: " + std::string(e.what()));
        }
        
        response.headers["Content-Type"] = "application/json";
        return response;
    });
    
    // 用户登出
    server.addRoute("POST", "/api/logout", [&db](const HttpRequest& req) -> HttpResponse {
        HttpResponse response;
        
        // 从Cookie中获取会话ID
        auto cookie_header = req.headers.find("Cookie");
        if (cookie_header != req.headers.end()) {
            std::string session_id;
            // 简单的Cookie解析
            size_t pos = cookie_header->second.find("session_id=");
            if (pos != std::string::npos) {
                pos += 11; // "session_id="的长度
                size_t end = cookie_header->second.find(";", pos);
                if (end == std::string::npos) end = cookie_header->second.length();
                session_id = cookie_header->second.substr(pos, end - pos);
                
                db.deleteSession(session_id);
            }
        }
        
        response.headers["Set-Cookie"] = "session_id=; Path=/; HttpOnly; Expires=Thu, 01 Jan 1970 00:00:00 GMT";
        response.body = JsonHelper::createSuccessResponse("登出成功");
        response.headers["Content-Type"] = "application/json";
        return response;
    });
    
    // === 文件管理相关API ===
    
    // 获取文件列表
    server.addRoute("GET", "/api/files", [&db](const HttpRequest& req) -> HttpResponse {
        HttpResponse response;
        
        int limit = 50;
        int offset = 0;
        std::string category = "";
        
        // 解析查询参数
        auto limit_param = req.params.find("limit");
        if (limit_param != req.params.end()) {
            limit = std::stoi(limit_param->second);
        }
        
        auto offset_param = req.params.find("offset");
        if (offset_param != req.params.end()) {
            offset = std::stoi(offset_param->second);
        }
        
        auto category_param = req.params.find("category");
        if (category_param != req.params.end()) {
            category = category_param->second;
        }
        
        std::vector<FileInfo> files;
        if (category.empty()) {
            files = db.getPublicFiles(limit, offset);
        } else {
            files = db.getFilesByCategory(category, limit, offset);
        }
        
        // 转换为JSON格式
        std::vector<std::map<std::string, std::string>> file_list;
        for (const auto& file : files) {
            std::map<std::string, std::string> file_data;
            file_data["id"] = std::to_string(file.id);
            file_data["filename"] = file.filename;
            file_data["file_type"] = file.file_type;
            file_data["file_size"] = std::to_string(file.file_size);
            file_data["upload_time"] = file.upload_time;
            file_data["category"] = file.category;
            file_data["download_count"] = std::to_string(file.download_count);
            file_list.push_back(file_data);
        }
        
        response.body = JsonHelper::createFileListResponse(file_list);
        response.headers["Content-Type"] = "application/json";
        return response;
    });
    
    // === 系统监控相关API ===
    
    // 获取系统状态
    server.addRoute("GET", "/api/system/status", [&sm](const HttpRequest& req) -> HttpResponse {
        HttpResponse response;
        
        SystemInfo info = sm.getSystemInfo();
        
        std::map<std::string, std::string> status;
        status["cpu_usage"] = std::to_string(info.cpu_usage);
        status["memory_usage"] = std::to_string(info.memory_usage);
        status["disk_usage"] = std::to_string(info.disk_usage);
        status["uptime"] = info.uptime;
        status["process_count"] = std::to_string(info.process_count);
        status["load_average"] = std::to_string(info.load_average_1);
        
        response.body = JsonHelper::createSystemStatusResponse(status);
        response.headers["Content-Type"] = "application/json";
        return response;
    });
    
    // 获取进程列表
    server.addRoute("GET", "/api/system/processes", [&sm](const HttpRequest& req) -> HttpResponse {
        HttpResponse response;
        
        auto processes = sm.getAllProcesses();
        
        JsonHelper::JsonArray process_array;
        for (const auto& proc : processes) {
            JsonHelper::JsonObject proc_obj;
            proc_obj["pid"] = proc.pid;
            proc_obj["name"] = proc.name;
            proc_obj["user"] = proc.user;
            proc_obj["state"] = proc.state;
            proc_obj["cpu_percent"] = proc.cpu_percent;
            proc_obj["memory_percent"] = proc.memory_percent;
            proc_obj["memory_usage"] = (int)proc.memory_usage;
            
            process_array.push_back(proc_obj);
        }
        
        response.body = JsonHelper::arrayToJson(process_array);
        response.headers["Content-Type"] = "application/json";
        return response;
    });
}

/**
 * 主函数
 */
int main() {
    std::cout << "=== g00j小站 文件共享系统 ===" << std::endl;
    std::cout << "正在启动服务器..." << std::endl;
    
    // 注册信号处理器
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // 初始化数据库
        std::cout << "初始化数据库..." << std::endl;
        g_database = std::make_unique<Database>("bin/g00j_share.db");
        if (!g_database->initialize()) {
            std::cerr << "数据库初始化失败！" << std::endl;
            return 1;
        }
        
        // 初始化文件管理器
        std::cout << "初始化文件管理器..." << std::endl;
        g_file_manager = std::make_unique<FileManager>();
        g_file_manager->setStorageRoot("shared");
        if (!g_file_manager->initialize()) {
            std::cerr << "文件管理器初始化失败！" << std::endl;
            return 1;
        }
        
        // 初始化系统监控器
        std::cout << "初始化系统监控器..." << std::endl;
        g_system_monitor = std::make_unique<SystemMonitor>();
        
        // 创建HTTP服务器
        std::cout << "创建HTTP服务器..." << std::endl;
        g_server = std::make_unique<HttpServer>(8080);
        g_server->setStaticRoot("static");
        
        // 注册API路由
        std::cout << "注册API路由..." << std::endl;
        registerApiRoutes(*g_server, *g_database, *g_file_manager, *g_system_monitor);
        
        // 启动服务器
        std::cout << "启动HTTP服务器..." << std::endl;
        if (!g_server->start()) {
            std::cerr << "服务器启动失败！" << std::endl;
            return 1;
        }
        
        std::cout << "========================================" << std::endl;
        std::cout << "服务器启动成功！" << std::endl;
        std::cout << "访问地址: http://localhost:8080" << std::endl;
        std::cout << "按 Ctrl+C 停止服务器" << std::endl;
        std::cout << "========================================" << std::endl;
        
        // 保持主线程运行
        while (g_server && g_server->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "发生异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 