#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <vector>

// HTTP 请求结构
struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> cookies;
};

// HTTP 响应结构
struct HttpResponse {
    int status_code = 200;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    void set_header(const std::string& key, const std::string& value);
    void set_cookie(const std::string& name, const std::string& value, 
                   const std::string& path = "/", int max_age = -1);
    void set_content_type(const std::string& type);
    void redirect(const std::string& location);
};

// 路由处理器类型
using RouteHandler = std::function<void(const HttpRequest&, HttpResponse&)>;

// HTTP 服务器类
class HttpServer {
public:
    HttpServer(int port = 8080);
    ~HttpServer();
    
    // 路由注册
    void get(const std::string& path, RouteHandler handler);
    void post(const std::string& path, RouteHandler handler);
    void put(const std::string& path, RouteHandler handler);
    void del(const std::string& path, RouteHandler handler);
    
    // 静态文件服务
    void serve_static(const std::string& path_prefix, const std::string& directory);
    
    // 启动服务器
    void start();
    void stop();
    
private:
    int port_;
    int server_fd_;
    bool running_;
    std::thread server_thread_;
    
    // 路由表
    std::unordered_map<std::string, std::unordered_map<std::string, RouteHandler>> routes_;
    
    // 静态文件配置
    std::unordered_map<std::string, std::string> static_dirs_;
    
    // 工作方法
    void server_loop();
    void handle_client(int client_fd);
    HttpRequest parse_request(const std::string& raw_request);
    std::string build_response(const HttpResponse& response);
    bool match_route(const std::string& pattern, const std::string& path, 
                    std::unordered_map<std::string, std::string>& params);
    void handle_static_file(const std::string& file_path, HttpResponse& response);
    std::string get_mime_type(const std::string& file_path);
    std::string url_decode(const std::string& encoded);
    std::string get_file_extension(const std::string& path);
}; 