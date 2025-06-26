#include "server.h"
#include "database.h"
#include "file_manager.h"
#include "system_monitor.h"
#include <iostream>
#include <csignal>
#include <memory>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include "json_helper.h"

// 全局对象
std::unique_ptr<HttpServer> server;
std::unique_ptr<Database> db;
std::unique_ptr<FileManager> file_manager;
std::unique_ptr<SystemMonitor> system_monitor;

// 信号处理
void signal_handler(int signal) {
    std::cout << "\n正在停止服务器..." << std::endl;
    if (server) {
        server->stop();
    }
    exit(0);
}

// JSON 响应辅助函数
std::string json_response(const std::string& content) {
    return content;
}

// 获取当前用户（从session）
std::unique_ptr<User> get_current_user(const HttpRequest& req) {
    auto session_cookie = req.cookies.find("session_id");
    if (session_cookie == req.cookies.end()) {
        return nullptr;
    }
    
    auto session = db->get_session(session_cookie->second);
    if (!session || !session->active) {
        return nullptr;
    }
    
    // 更新会话访问时间
    db->update_session_access(session_cookie->second);
    
    return db->get_user_by_id(session->user_id);
}

// 检查管理员权限
bool is_admin(const HttpRequest& req) {
    auto user = get_current_user(req);
    return user && user->role == "admin";
}

// 路由处理函数
void setup_routes() {
    // 静态文件服务
    server->serve_static("/static", "../static");
    server->serve_static("/shared", "../shared");
    
    // 主页
    server->get("/", [](const HttpRequest& req, HttpResponse& resp) {
        auto files = db->get_public_files(10); // 最新10个文件
        
        // 加载主页HTML模板
        std::ifstream html_file("../static/index.html");
        if (html_file.is_open()) {
            std::ostringstream content;
            content << html_file.rdbuf();
            resp.body = content.str();
        } else {
            resp.body = "<html><body><h1>Welcome to g00j File Share</h1><p>Index.html not found</p></body></html>";
        }
        
        resp.set_content_type("text/html; charset=utf-8");
    });
    
    // API: 文件列表
    server->get("/api/files", [](const HttpRequest& req, HttpResponse& resp) {
        auto files = db->get_public_files();
        
        // 手动构建JSON字符串，因为我们的JsonBuilder不太适合复杂嵌套结构
        std::ostringstream json;
        json << "{\"files\":[";
        
        bool first = true;
        for (const auto& file : files) {
            if (!first) json << ",";
            json << "{"
                 << "\"id\":" << file.id << ","
                 << "\"filename\":\"" << file.original_name << "\","
                 << "\"path\":\"/shared/" << file.filename << "\","
                 << "\"category\":\"" << file.category << "\","
                 << "\"size\":" << file.file_size << ","
                 << "\"upload_time\":\"" << file.upload_time << "\","
                 << "\"is_public\":" << (file.is_public ? "true" : "false")
                 << "}";
            first = false;
        }
        
        json << "]}";
        
        resp.set_content_type("application/json");
        resp.body = json.str();
    });
    
    // API: 用户登录
    server->post("/api/login", [](const HttpRequest& req, HttpResponse& resp) {
        JsonParser parser(req.body);
        
        if (!parser.has_key("username") || !parser.has_key("password")) {
            resp.status_code = 400;
            JsonBuilder error;
            error.add("error", "Invalid JSON");
            resp.body = error.build();
            resp.set_content_type("application/json");
            return;
        }
        
        std::string username = parser.get_string("username");
        std::string password = parser.get_string("password");
        
        if (db->verify_password(username, password)) {
            auto user = db->get_user(username);
            if (user) {
                std::string session_id = db->create_session(user->id);
                resp.set_cookie("session_id", session_id, "/", 86400 * 7); // 7天
                
                JsonBuilder response;
                response.add("success", true);
                response.add_object("user");
                response.add("id", user->id);
                response.add("username", user->username);
                response.add("role", user->role);
                response.end_object();
                resp.body = response.build();
            }
        } else {
            resp.status_code = 401;
            JsonBuilder error;
            error.add("error", "Invalid credentials");
            resp.body = error.build();
        }
        
        resp.set_content_type("application/json");
    });
    
    // API: 获取当前用户信息
    server->get("/api/user", [](const HttpRequest& req, HttpResponse& resp) {
        auto user = get_current_user(req);
        if (user) {
            JsonBuilder response;
            response.add("success", true);
            response.add_object("user");
            response.add("id", user->id);
            response.add("username", user->username);
            response.add("role", user->role);
            response.end_object();
            resp.body = response.build();
        } else {
            resp.status_code = 401;
            JsonBuilder error;
            error.add("success", false);
            error.add("error", "Not authenticated");
            resp.body = error.build();
        }
        
        resp.set_content_type("application/json");
    });

    // API: 用户注册
    server->post("/api/register", [](const HttpRequest& req, HttpResponse& resp) {
        JsonParser parser(req.body);
        
        if (!parser.has_key("username") || !parser.has_key("password")) {
            resp.status_code = 400;
            JsonBuilder error;
            error.add("error", "Invalid JSON");
            resp.body = error.build();
            resp.set_content_type("application/json");
            return;
        }
        
        std::string username = parser.get_string("username");
        std::string password = parser.get_string("password");
        
        if (username.empty() || password.empty()) {
            resp.status_code = 400;
            JsonBuilder error;
            error.add("error", "Username and password required");
            resp.body = error.build();
            resp.set_content_type("application/json");
            return;
        }
        
        if (db->create_user(username, password)) {
            JsonBuilder response;
            response.add("success", true);
            response.add("message", "User created successfully");
            resp.body = response.build();
        } else {
            resp.status_code = 409;
            JsonBuilder error;
            error.add("error", "Username already exists");
            resp.body = error.build();
        }
        
        resp.set_content_type("application/json");
    });
    
    // API: 文件上传
    server->post("/api/upload", [](const HttpRequest& req, HttpResponse& resp) {
        auto user = get_current_user(req);
        if (!user) {
            resp.status_code = 401;
            JsonBuilder error;
            error.add("error", "Login required");
            resp.body = error.build();
            resp.set_content_type("application/json");
            return;
        }
        
        // 这里需要处理multipart/form-data上传
        // 简化处理，实际需要解析multipart数据
        
        JsonBuilder response;
        response.add("success", true);
        response.add("message", "File uploaded successfully");
        resp.body = response.build();
        resp.set_content_type("application/json");
    });
    
    // API: 系统监控（管理员）
    server->get("/api/system", [](const HttpRequest& req, HttpResponse& resp) {
        if (!is_admin(req)) {
            resp.status_code = 403;
            JsonBuilder error;
            error.add("error", "Admin access required");
            resp.body = error.build();
            resp.set_content_type("application/json");
            return;
        }
        
        auto sys_info = system_monitor->get_system_info();
        JsonBuilder data;
        data.add("cpu_usage", sys_info.cpu_usage);
        data.add("memory_total", sys_info.total_memory_kb);
        data.add("memory_used", sys_info.used_memory_kb);
        data.add("memory_usage_percent", sys_info.memory_usage_percent);
        data.add("disk_total", sys_info.total_disk_kb);
        data.add("disk_used", sys_info.used_disk_kb);
        data.add("disk_usage_percent", sys_info.disk_usage_percent);
        data.add("uptime", sys_info.uptime);
        data.add("process_count", sys_info.process_count);
        
        resp.set_content_type("application/json");
        resp.body = data.build();
    });
    
    // API: 进程列表（管理员）
    server->get("/api/processes", [](const HttpRequest& req, HttpResponse& resp) {
        if (!is_admin(req)) {
            resp.status_code = 403;
            JsonBuilder error;
            error.add("error", "Admin access required");
            resp.body = error.build();
            resp.set_content_type("application/json");
            return;
        }
        
        auto processes = system_monitor->get_process_list();
        
        // 手动构建JSON字符串
        std::ostringstream json;
        json << "{\"processes\":[";
        
        bool first = true;
        for (const auto& proc : processes) {
            if (!first) json << ",";
            json << "{"
                 << "\"pid\":" << proc.pid << ","
                 << "\"name\":\"" << proc.name << "\","
                 << "\"user\":\"" << proc.user << "\","
                 << "\"cpu_percent\":" << proc.cpu_percent << ","
                 << "\"memory_kb\":" << proc.memory_kb << ","
                 << "\"status\":\"" << proc.status << "\""
                 << "}";
            first = false;
        }
        
        json << "]}";
        
        resp.set_content_type("application/json");
        resp.body = json.str();
    });

    // 页面路由
    server->get("/files", [](const HttpRequest& req, HttpResponse& resp) {
        // 返回文件浏览页面（可以使用相同的index.html）
        std::ifstream html_file("../static/index.html");
        if (html_file.is_open()) {
            std::ostringstream content;
            content << html_file.rdbuf();
            resp.body = content.str();
        } else {
            resp.body = "<html><body><h1>Files Page</h1></body></html>";
        }
        resp.set_content_type("text/html; charset=utf-8");
    });

    server->get("/upload", [](const HttpRequest& req, HttpResponse& resp) {
        // 返回上传页面
        std::ifstream html_file("../static/index.html");
        if (html_file.is_open()) {
            std::ostringstream content;
            content << html_file.rdbuf();
            resp.body = content.str();
        } else {
            resp.body = "<html><body><h1>Upload Page</h1></body></html>";
        }
        resp.set_content_type("text/html; charset=utf-8");
    });

    server->get("/admin", [](const HttpRequest& req, HttpResponse& resp) {
        // 返回管理页面
        std::ifstream html_file("../static/index.html");
        if (html_file.is_open()) {
            std::ostringstream content;
            content << html_file.rdbuf();
            resp.body = content.str();
        } else {
            resp.body = "<html><body><h1>Admin Page</h1></body></html>";
        }
        resp.set_content_type("text/html; charset=utf-8");
    });

    server->get("/profile", [](const HttpRequest& req, HttpResponse& resp) {
        // 返回个人资料页面
        std::ifstream html_file("../static/index.html");
        if (html_file.is_open()) {
            std::ostringstream content;
            content << html_file.rdbuf();
            resp.body = content.str();
        } else {
            resp.body = "<html><body><h1>Profile Page</h1></body></html>";
        }
        resp.set_content_type("text/html; charset=utf-8");
    });

    server->get("/my-files", [](const HttpRequest& req, HttpResponse& resp) {
        // 返回我的文件页面
        std::ifstream html_file("../static/index.html");
        if (html_file.is_open()) {
            std::ostringstream content;
            content << html_file.rdbuf();
            resp.body = content.str();
        } else {
            resp.body = "<html><body><h1>My Files Page</h1></body></html>";
        }
        resp.set_content_type("text/html; charset=utf-8");
    });
}

int main() {
    std::cout << "=== g00j小站 文件共享系统 ===" << std::endl;
    std::cout << "正在初始化..." << std::endl;
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        // 初始化组件
        db = std::make_unique<Database>();
        if (!db->init()) {
            std::cerr << "数据库初始化失败!" << std::endl;
            return 1;
        }
        
        // 创建默认管理员账户
        if (!db->get_user("admin")) {
            db->create_user("admin", "admin123", "admin");
            std::cout << "创建默认管理员账户: admin / admin123" << std::endl;
        }
        
        file_manager = std::make_unique<FileManager>();
        system_monitor = std::make_unique<SystemMonitor>();
        server = std::make_unique<HttpServer>(8080);
        
        // 设置路由
        setup_routes();
        
        std::cout << "服务器启动成功!" << std::endl;
        std::cout << "访问地址: http://localhost:8080" << std::endl;
        std::cout << "按 Ctrl+C 停止服务" << std::endl;
        
        // 启动服务器
        server->start();
        
        // 保持程序运行，等待信号
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "启动失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 