#pragma once

#include <string>
#include <map>
#include <functional>
#include <thread>
#include <vector>
#include <mutex>

// HTTP请求结构
struct HttpRequest {
    std::string method;     // GET, POST, etc.
    std::string path;       // URL路径
    std::string version;    // HTTP版本
    std::string query_string; // 查询字符串
    std::map<std::string, std::string> headers;  // 请求头
    std::string body;       // 请求体
    std::map<std::string, std::string> params;   // 查询参数
};

// HTTP响应结构
struct HttpResponse {
    int status_code;        // 状态码
    std::string status_text; // 状态文本
    std::map<std::string, std::string> headers;  // 响应头
    std::string body;       // 响应体
    
    HttpResponse() : status_code(200), status_text("OK") {}
};

// 路由处理器类型定义
using RouteHandler = std::function<void(const HttpRequest&, HttpResponse&)>;

class HttpServer {
public:

private:
    int port_;
    int server_fd_;
    int server_socket;
    int port;
    std::map<std::string, RouteHandler> routes_;
    std::map<std::string, RouteHandler> routes;
    std::map<std::string, RouteHandler> post_routes;
    std::string static_root_;
    std::mutex routes_mutex_;
    bool running_;
    bool running;

public:
    HttpServer(int port);
    ~HttpServer();

    bool start();    // 修改返回类型为bool
    void stop();
    bool is_running() const;
    
    void addRoute(const std::string& method, const std::string& path, RouteHandler handler);
    void setStaticRoot(const std::string& root);
    
    void add_route(const std::string& path, RouteHandler handler);
    void add_post_route(const std::string& path, RouteHandler handler);
    
    std::map<std::string, std::string> parse_query_params(const std::string& query_string);

private:
    void handle_client(int client_socket);
    
    // HTTP解析和生成
    HttpRequest parse_request(const std::string& raw_request);
    std::string generate_response(const HttpResponse& response);
    HttpResponse handleRoute(const HttpRequest& request);
    
    // 静态文件服务
    bool handle_static_file(const std::string& path, HttpResponse& response);
    std::string url_decode(const std::string& str);
    
    std::string get_mime_type(const std::string& path);
    std::string serve_static_file(const std::string& path);
    std::string parse_request_body(const std::string& request);
}; 