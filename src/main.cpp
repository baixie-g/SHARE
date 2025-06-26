#include <iostream>
#include <signal.h>
#include <random>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <sys/stat.h>
#include "server.h"
#include "database.h"
#include "file_manager.h"
#include "json_helper.h"
#include "system_monitor.h"

// 全局变量
HttpServer* g_server = nullptr;
Database* g_database = nullptr;
FileManager* g_file_manager = nullptr;

// 信号处理函数
void signal_handler(int signal) {
    std::cout << "\n收到信号 " << signal << "，正在关闭服务器..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(0);
}

// 生成随机session ID
std::string generate_session_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        int hex = dis(gen);
        ss << std::hex << hex;
    }
    return ss.str();
}

// 从请求头中提取session ID
std::string get_session_from_cookies(const std::string& cookie_header) {
    size_t pos = cookie_header.find("session_id=");
    if (pos != std::string::npos) {
        pos += 11; // "session_id="的长度
        size_t end = cookie_header.find(";", pos);
        if (end == std::string::npos) {
            end = cookie_header.length();
        }
        return cookie_header.substr(pos, end - pos);
    }
    return "";
}

// 验证用户权限
bool check_admin_permission(const std::string& session_id) {
    if (session_id.empty()) return false;
    Session session = g_database->get_session(session_id);
    return session.role == "admin";
}

// 前向声明
std::string handle_login(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_register(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_logout(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_get_files(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_upload(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_system_status(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_processes(const std::string& body, const std::map<std::string, std::string>& params);

// 包装函数：将旧的路由处理器适配为新的签名
void handle_login_route(const HttpRequest& request, HttpResponse& response) {
    std::string result = handle_login(request.body, request.params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_register_route(const HttpRequest& request, HttpResponse& response) {
    std::string result = handle_register(request.body, request.params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_logout_route(const HttpRequest& request, HttpResponse& response) {
    std::string result = handle_logout(request.body, request.params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_get_files_route(const HttpRequest& request, HttpResponse& response) {
    std::string result = handle_get_files(request.body, request.params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_upload_route(const HttpRequest& request, HttpResponse& response) {
    // 将headers和params合并传递给handle_upload
    std::map<std::string, std::string> combined_params = request.params;
    for (const auto& header : request.headers) {
        combined_params[header.first] = header.second;
    }
    
    std::string result = handle_upload(request.body, combined_params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_system_status_route(const HttpRequest& request, HttpResponse& response) {
    std::string result = handle_system_status(request.body, request.params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_processes_route(const HttpRequest& request, HttpResponse& response) {
    std::string result = handle_processes(request.body, request.params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

// 用户登录
std::string handle_login(const std::string& body, const std::map<std::string, std::string>& params) {
    try {
        auto form_data = JsonHelper::parse_form_data(body);
        std::string username = form_data["username"];
        std::string password = form_data["password"];
        
        if (username.empty() || password.empty()) {
            return JsonHelper::error_response("Username and password are required");
        }
        
        // 调试：检查密码验证
        bool password_valid = g_database->verify_password(username, password);
        if (password_valid) {
            User user = g_database->get_user(username);
            std::string session_id = generate_session_id();
            
            if (g_database->create_session(session_id, username, user.role)) {
                return JsonHelper::success_response("Login successful");
            } else {
                return JsonHelper::error_response("Session creation failed");
            }
        }
        
        return JsonHelper::error_response("Invalid username or password");
        
    } catch (const std::exception& e) {
        return JsonHelper::error_response("Internal server error");
    }
}

// 用户注册
std::string handle_register(const std::string& body, const std::map<std::string, std::string>& params) {
    auto form_data = JsonHelper::parse_form_data(body);
    std::string username = form_data["username"];
    std::string password = form_data["password"];
    
    if (username.empty() || password.empty()) {
        return JsonHelper::error_response("Username and password are required");
    }
    
    if (username.length() < 3 || password.length() < 6) {
        return JsonHelper::error_response("Username must be at least 3 characters, password at least 6");
    }
    
    if (g_database->create_user(username, password)) {
        return JsonHelper::success_response("Registration successful");
    }
    
    return JsonHelper::error_response("Username already exists");
}

// 用户登出
std::string handle_logout(const std::string& body, const std::map<std::string, std::string>& params) {
    // 这里应该从Cookie中获取session_id，简化处理
    return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nSet-Cookie: session_id=; Path=/; Max-Age=0\r\n\r\n" + 
           JsonHelper::success_response("Logout successful");
}

// 获取文件列表
std::string handle_get_files(const std::string& body, const std::map<std::string, std::string>& params) {
    int page = 1;
    int limit = 20;
    std::string category = "";
    
    auto it = params.find("page");
    if (it != params.end()) {
        page = std::stoi(it->second);
    }
    
    it = params.find("limit");
    if (it != params.end()) {
        limit = std::stoi(it->second);
    }
    
    it = params.find("category");
    if (it != params.end()) {
        category = it->second;
    }
    
    std::vector<FileInfo> files = g_database->get_files(page, limit, category);
    int total = g_database->get_total_files(category);
    
    std::string files_json = JsonHelper::serialize_files(files);
    return JsonHelper::paginated_response(files_json, total, page, limit);
}

// 简化的multipart/form-data解析
std::map<std::string, std::string> parse_multipart_fields(const std::string& body, const std::string& boundary) {
    std::map<std::string, std::string> fields;
    
    size_t pos = 0;
    while (pos < body.length()) {
        size_t boundary_pos = body.find("--" + boundary, pos);
        if (boundary_pos == std::string::npos) break;
        
        size_t content_start = body.find("\r\n\r\n", boundary_pos);
        if (content_start == std::string::npos) break;
        
        std::string headers = body.substr(boundary_pos, content_start - boundary_pos);
        content_start += 4; // 跳过"\r\n\r\n"
        
        size_t content_end = body.find("\r\n--" + boundary, content_start);
        if (content_end == std::string::npos) content_end = body.length();
        
        std::string content = body.substr(content_start, content_end - content_start);
        
        // 解析字段名
        size_t name_pos = headers.find("name=\"");
        if (name_pos != std::string::npos) {
            name_pos += 6;
            size_t name_end = headers.find("\"", name_pos);
            if (name_end != std::string::npos) {
                std::string field_name = headers.substr(name_pos, name_end - name_pos);
                fields[field_name] = content;
            }
        }
        
        pos = content_end;
    }
    
    return fields;
}

// 文件上传
std::string handle_upload(const std::string& body, const std::map<std::string, std::string>& params) {
    try {
        // 从Content-Type中提取boundary (检查多种格式)
        auto content_type_it = params.find("Content-Type");
        if (content_type_it == params.end()) {
            content_type_it = params.find("content-type");
        }
        if (content_type_it == params.end()) {
            // 调试：打印所有headers
            std::string debug_headers = "Headers: ";
            for (const auto& p : params) {
                debug_headers += p.first + "=" + p.second + "; ";
            }
            return JsonHelper::error_response("Missing Content-Type header. " + debug_headers);
        }
        
        std::string content_type = content_type_it->second;
        size_t boundary_pos = content_type.find("boundary=");
        if (boundary_pos == std::string::npos) {
            return JsonHelper::error_response("Missing boundary in Content-Type");
        }
        
        std::string boundary = content_type.substr(boundary_pos + 9);
        
        // 解析multipart数据
        auto fields = parse_multipart_fields(body, boundary);
        
        auto file_it = fields.find("file");
        auto category_it = fields.find("category");
        
        if (file_it == fields.end()) {
            return JsonHelper::error_response("No file provided");
        }
        
        std::string category = "others";
        if (category_it != fields.end()) {
            category = category_it->second;
        }
        
        // 简化实现：生成文件名和保存文件
        std::string filename = "uploaded_" + std::to_string(time(nullptr)) + ".txt";
        std::string filepath = "shared/" + category + "/" + filename;
        
        // 确保目录存在
        std::string dir_path = "shared/" + category;
        mkdir(dir_path.c_str(), 0755);
        
        // 保存文件
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return JsonHelper::error_response("Failed to save file");
        }
        
        file.write(file_it->second.c_str(), file_it->second.length());
        file.close();
        
        // 添加到数据库
        long file_size = file_it->second.length();
        bool success = g_database->addFile(filename, filepath, "application/octet-stream", file_size, 1, category);
        
        if (success) {
            return JsonHelper::success_response("File uploaded successfully");
        } else {
            return JsonHelper::error_response("Failed to save file info to database");
        }
        
    } catch (const std::exception& e) {
        return JsonHelper::error_response("Upload failed: Internal error");
    }
}

// 系统状态监控
std::string handle_system_status(const std::string& body, const std::map<std::string, std::string>& params) {
    // 检查管理员权限（简化处理）
    auto status = SystemMonitor::get_system_status();
    return JsonHelper::serialize_system_status(status);
}

// 进程列表
std::string handle_processes(const std::string& body, const std::map<std::string, std::string>& params) {
    // 检查管理员权限（简化处理）
    auto processes = SystemMonitor::get_processes();
    return JsonHelper::serialize_processes(processes);
}

int main() {
    std::cout << "启动 g00j小站 文件共享系统..." << std::endl;
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化数据库
    g_database = new Database("bin/g00j_share.db");
    if (!g_database->initialize()) {
        std::cerr << "数据库初始化失败" << std::endl;
        return 1;
    }
    
    // 创建默认管理员账户
    g_database->create_user("admin", "admin123", "admin");
    
    // 初始化文件管理器
    g_file_manager = new FileManager("shared");
    if (!g_file_manager->create_directories()) {
        std::cerr << "文件目录创建失败" << std::endl;
        return 1;
    }
    
    // 启动HTTP服务器
    g_server = new HttpServer(8080);
    
    // 设置静态文件目录
    g_server->setStaticRoot("static");
    
    // 注册API路由
    g_server->add_post_route("/api/login", handle_login_route);
    g_server->add_post_route("/api/register", handle_register_route);
    g_server->add_post_route("/api/logout", handle_logout_route);
    g_server->add_post_route("/api/upload", handle_upload_route);
    
    g_server->add_route("/api/files", handle_get_files_route);
    g_server->add_route("/api/system/status", handle_system_status_route);
    g_server->add_route("/api/system/processes", handle_processes_route);
    
    std::cout << "服务器启动成功，访问地址: http://localhost:8080" << std::endl;
    std::cout << "默认管理员账户: admin / admin123" << std::endl;
    
    // 启动服务器
    if (!g_server->start()) {
        std::cerr << "服务器启动失败" << std::endl;
        return 1;
    }
    
    // 等待服务器运行
    std::cout << "按 Ctrl+C 停止服务器..." << std::endl;
    while (g_server->is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 清理资源
    delete g_server;
    delete g_database;
    delete g_file_manager;
    
    return 0;
} 