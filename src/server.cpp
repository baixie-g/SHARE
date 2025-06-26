#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <regex>

// HttpResponse 方法实现
void HttpResponse::set_header(const std::string& key, const std::string& value) {
    headers[key] = value;
}

void HttpResponse::set_cookie(const std::string& name, const std::string& value, 
                             const std::string& path, int max_age) {
    std::string cookie_value = name + "=" + value + "; Path=" + path;
    if (max_age > 0) {
        cookie_value += "; Max-Age=" + std::to_string(max_age);
    }
    headers["Set-Cookie"] = cookie_value;
}

void HttpResponse::set_content_type(const std::string& type) {
    headers["Content-Type"] = type;
}

void HttpResponse::redirect(const std::string& location) {
    status_code = 302;
    headers["Location"] = location;
}

// HttpServer 实现
HttpServer::HttpServer(int port) : port_(port), server_fd_(-1), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::get(const std::string& path, RouteHandler handler) {
    routes_["GET"][path] = handler;
}

void HttpServer::post(const std::string& path, RouteHandler handler) {
    routes_["POST"][path] = handler;
}

void HttpServer::put(const std::string& path, RouteHandler handler) {
    routes_["PUT"][path] = handler;
}

void HttpServer::del(const std::string& path, RouteHandler handler) {
    routes_["DELETE"][path] = handler;
}

void HttpServer::serve_static(const std::string& path_prefix, const std::string& directory) {
    static_dirs_[path_prefix] = directory;
}

void HttpServer::start() {
    // 创建socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // 设置socket选项
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }
    
    // 绑定地址
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }
    
    // 监听连接
    if (listen(server_fd_, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
    
    running_ = true;
    server_thread_ = std::thread(&HttpServer::server_loop, this);
}

void HttpServer::stop() {
    running_ = false;
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void HttpServer::server_loop() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }
        
        // 处理客户端连接（在新线程中）
        std::thread(&HttpServer::handle_client, this, client_fd).detach();
    }
}

void HttpServer::handle_client(int client_fd) {
    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    std::string raw_request(buffer);
    
    try {
        HttpRequest request = parse_request(raw_request);
        HttpResponse response;
        
        // 查找路由
        bool route_found = false;
        auto method_routes = routes_.find(request.method);
        if (method_routes != routes_.end()) {
            for (const auto& route : method_routes->second) {
                std::unordered_map<std::string, std::string> params;
                if (match_route(route.first, request.path, params)) {
                    request.params = params;
                    route.second(request, response);
                    route_found = true;
                    break;
                }
            }
        }
        
        // 检查静态文件
        if (!route_found) {
            for (const auto& static_dir : static_dirs_) {
                if (request.path.find(static_dir.first) == 0) {
                    std::string file_path = static_dir.second + request.path.substr(static_dir.first.length());
                    handle_static_file(file_path, response);
                    route_found = true;
                    break;
                }
            }
        }
        
        // 404
        if (!route_found) {
            response.status_code = 404;
            response.body = "404 Not Found";
            response.set_content_type("text/plain");
        }
        
        // 发送响应
        std::string response_str = build_response(response);
        send(client_fd, response_str.c_str(), response_str.length(), 0);
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling request: " << e.what() << std::endl;
        
        std::string error_response = "HTTP/1.1 500 Internal Server Error\r\n"
                                   "Content-Type: text/plain\r\n"
                                   "Content-Length: 21\r\n"
                                   "\r\n"
                                   "500 Internal Server Error";
        send(client_fd, error_response.c_str(), error_response.length(), 0);
    }
    
    close(client_fd);
}

HttpRequest HttpServer::parse_request(const std::string& raw_request) {
    HttpRequest request;
    std::istringstream stream(raw_request);
    std::string line;
    
    // 解析请求行
    if (std::getline(stream, line)) {
        std::istringstream line_stream(line);
        line_stream >> request.method >> request.path >> request.version;
    }
    
    // 解析头部
    while (std::getline(stream, line) && line != "\r") {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            // 去除前后空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t\r") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r") + 1);
            
            request.headers[key] = value;
        }
    }
    
    // 解析Cookie
    auto cookie_header = request.headers.find("Cookie");
    if (cookie_header != request.headers.end()) {
        std::istringstream cookie_stream(cookie_header->second);
        std::string cookie_pair;
        while (std::getline(cookie_stream, cookie_pair, ';')) {
            size_t eq_pos = cookie_pair.find('=');
            if (eq_pos != std::string::npos) {
                std::string name = cookie_pair.substr(0, eq_pos);
                std::string value = cookie_pair.substr(eq_pos + 1);
                
                // 去除前后空格
                name.erase(0, name.find_first_not_of(" \t"));
                name.erase(name.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                request.cookies[name] = value;
            }
        }
    }
    
    // 解析查询参数
    size_t query_pos = request.path.find('?');
    if (query_pos != std::string::npos) {
        std::string query = request.path.substr(query_pos + 1);
        request.path = request.path.substr(0, query_pos);
        
        std::istringstream query_stream(query);
        std::string param;
        while (std::getline(query_stream, param, '&')) {
            size_t eq_pos = param.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = url_decode(param.substr(0, eq_pos));
                std::string value = url_decode(param.substr(eq_pos + 1));
                request.params[key] = value;
            }
        }
    }
    
    // 读取请求体
    std::string body;
    std::string remaining;
    while (std::getline(stream, remaining)) {
        body += remaining + "\n";
    }
    if (!body.empty()) {
        body.pop_back(); // 移除最后的换行符
    }
    request.body = body;
    
    return request;
}

std::string HttpServer::build_response(const HttpResponse& response) {
    std::ostringstream response_stream;
    
    // 状态行
    response_stream << "HTTP/1.1 " << response.status_code;
    switch (response.status_code) {
        case 200: response_stream << " OK"; break;
        case 302: response_stream << " Found"; break;
        case 400: response_stream << " Bad Request"; break;
        case 401: response_stream << " Unauthorized"; break;
        case 403: response_stream << " Forbidden"; break;
        case 404: response_stream << " Not Found"; break;
        case 500: response_stream << " Internal Server Error"; break;
        default: response_stream << " Unknown"; break;
    }
    response_stream << "\r\n";
    
    // 头部
    response_stream << "Content-Length: " << response.body.length() << "\r\n";
    for (const auto& header : response.headers) {
        response_stream << header.first << ": " << header.second << "\r\n";
    }
    response_stream << "\r\n";
    
    // 响应体
    response_stream << response.body;
    
    return response_stream.str();
}

bool HttpServer::match_route(const std::string& pattern, const std::string& path, 
                           std::unordered_map<std::string, std::string>& params) {
    // 简单匹配（可以扩展为支持参数的路由匹配）
    if (pattern == path) {
        return true;
    }
    
    // 支持通配符匹配
    if (pattern.find('*') != std::string::npos) {
        std::string regex_pattern = std::regex_replace(pattern, std::regex("\\*"), ".*");
        return std::regex_match(path, std::regex(regex_pattern));
    }
    
    return false;
}

void HttpServer::handle_static_file(const std::string& file_path, HttpResponse& response) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        response.status_code = 404;
        response.body = "File not found";
        response.set_content_type("text/plain");
        return;
    }
    
    // 读取文件内容
    std::ostringstream file_stream;
    file_stream << file.rdbuf();
    response.body = file_stream.str();
    
    // 设置Content-Type
    response.set_content_type(get_mime_type(file_path));
    
    // 设置缓存头
    response.set_header("Cache-Control", "public, max-age=3600");
}

std::string HttpServer::get_mime_type(const std::string& file_path) {
    std::string extension = get_file_extension(file_path);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    static std::unordered_map<std::string, std::string> mime_types = {
        {".html", "text/html; charset=utf-8"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".mp4", "video/mp4"},
        {".avi", "video/x-msvideo"},
        {".mkv", "video/x-matroska"},
        {".txt", "text/plain; charset=utf-8"},
        {".md", "text/markdown; charset=utf-8"},
        {".pdf", "application/pdf"},
        {".zip", "application/zip"},
        {".tar", "application/x-tar"},
        {".gz", "application/gzip"}
    };
    
    auto it = mime_types.find(extension);
    return (it != mime_types.end()) ? it->second : "application/octet-stream";
}

std::string HttpServer::url_decode(const std::string& encoded) {
    std::string decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            std::string hex = encoded.substr(i + 1, 2);
            char ch = static_cast<char>(std::stoi(hex, nullptr, 16));
            decoded += ch;
            i += 2;
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    return decoded;
}

std::string HttpServer::get_file_extension(const std::string& path) {
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    return path.substr(dot_pos);
} 