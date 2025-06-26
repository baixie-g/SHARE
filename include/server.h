#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <mutex>

// HTTP请求结构
struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> params;
    std::string body;
    std::string remote_addr;
};

// HTTP响应结构
struct HttpResponse {
    int status_code = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    std::vector<uint8_t> binary_data;
    bool is_binary = false;
};

// 路由处理函数类型
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

/**
 * HTTP服务器类
 * 提供HTTP服务，支持静态文件服务和API路由
 */
class HttpServer {
public:
    HttpServer(int port = 8080);
    ~HttpServer();

    // 启动服务器
    bool start();
    
    // 停止服务器
    void stop();
    
    // 添加路由处理器
    void addRoute(const std::string& method, const std::string& path, RouteHandler handler);
    
    // 设置静态文件根目录
    void setStaticRoot(const std::string& root);
    
    // 运行状态检查
    bool isRunning() const { return running_; }

private:
    int port_;
    int server_fd_;
    bool running_;
    std::string static_root_;
    std::map<std::string, RouteHandler> routes_;
    std::vector<std::thread> worker_threads_;
    std::mutex routes_mutex_;
    
    // 创建套接字
    bool createSocket();
    
    // 绑定和监听
    bool bindAndListen();
    
    // 处理客户端连接
    void handleClient(int client_fd);
    
    // 解析HTTP请求
    HttpRequest parseRequest(const std::string& request_data, const std::string& remote_addr);
    
    // 生成HTTP响应
    std::string generateResponse(const HttpResponse& response);
    
    // 处理静态文件请求
    HttpResponse handleStaticFile(const std::string& path);
    
    // 处理API路由
    HttpResponse handleRoute(const HttpRequest& request);
    
    // URL解码
    std::string urlDecode(const std::string& str);
    
    // 获取MIME类型
    std::string getMimeType(const std::string& filename);
    
    // 解析查询参数
    std::map<std::string, std::string> parseQueryParams(const std::string& query);
    
    // 工作线程函数
    void workerThread();
};

#endif // SERVER_H 