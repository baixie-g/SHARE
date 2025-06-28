#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <fstream>
#include <cstring>
#include <vector>

HttpServer::HttpServer(int port) : port_(port), server_fd_(-1), running_(false) {
}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "创建套接字失败" << std::endl;
        return false;
    }
    
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "设置套接字选项失败" << std::endl;
        close(server_fd_);
        return false;
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) == -1) {
        std::cerr << "绑定地址失败" << std::endl;
        close(server_fd_);
        return false;
    }
    
    if (listen(server_fd_, 10) == -1) {
        std::cerr << "监听失败" << std::endl;
        close(server_fd_);
        return false;
    }
    
    running_ = true;
    
    std::thread accept_thread([this]() {
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd == -1) {
                if (running_) {
                    std::cerr << "接受连接失败" << std::endl;
                }
                continue;
            }
            
            std::thread client_thread([this, client_fd]() {
                handle_client(client_fd);
            });
            client_thread.detach();
        }
    });
    accept_thread.detach();
    
    std::cout << "HTTP服务器在端口 " << port_ << " 启动成功" << std::endl;
    return true;
}

void HttpServer::stop() {
    if (running_) {
        running_ = false;
        
        if (server_fd_ >= 0) {
            close(server_fd_);
            server_fd_ = -1;
        }
    }
}

bool HttpServer::is_running() const {
    return running_;
}

void HttpServer::addRoute(const std::string& method, const std::string& path, RouteHandler handler) {
    std::lock_guard<std::mutex> lock(routes_mutex_);
    routes_[method + " " + path] = handler;
}

void HttpServer::setStaticRoot(const std::string& root) {
    static_root_ = root;
}

void HttpServer::handle_client(int client_fd) {
    // 先读取HTTP头部来确定Content-Length
    std::string headers;
    char buffer[4096];
    ssize_t total_read = 0;
    bool headers_complete = false;
    
    // 读取并解析HTTP头部
    while (!headers_complete && total_read < sizeof(buffer) - 1) {
        ssize_t bytes_read = recv(client_fd, buffer + total_read, sizeof(buffer) - 1 - total_read, 0);
        if (bytes_read <= 0) {
            close(client_fd);
            return;
        }
        
        total_read += bytes_read;
        buffer[total_read] = '\0';
        
        // 检查是否已读取完整的HTTP头部（以\r\n\r\n结束）
        std::string current_data(buffer, total_read);
        size_t header_end = current_data.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            headers = current_data.substr(0, header_end + 4);
            headers_complete = true;
        }
    }
    
    if (!headers_complete) {
        close(client_fd);
        return;
    }
    
    // 解析Content-Length
    size_t content_length = 0;
    size_t cl_pos = headers.find("Content-Length:");
    if (cl_pos == std::string::npos) {
        cl_pos = headers.find("content-length:");
    }
    
    if (cl_pos != std::string::npos) {
        size_t value_start = headers.find(":", cl_pos) + 1;
        size_t value_end = headers.find("\r\n", value_start);
        if (value_end != std::string::npos) {
            std::string length_str = headers.substr(value_start, value_end - value_start);
            // 移除前导空格
            length_str.erase(0, length_str.find_first_not_of(' '));
            content_length = std::stoull(length_str);
        }
    }
    
    // 构建完整的HTTP请求
    std::string raw_request = headers;
    
    // 如果有body内容，继续读取
    if (content_length > 0) {
        // 检查header中是否已经包含了部分body
        size_t header_end = headers.find("\r\n\r\n");
        size_t body_in_buffer = total_read - (header_end + 4);
        
        // 为body数据分配足够的空间
        std::vector<char> body_buffer(content_length);
        size_t body_read = 0;
        
        // 如果缓冲区中已有部分body数据
        if (body_in_buffer > 0) {
            size_t copy_size = std::min(body_in_buffer, content_length);
            memcpy(body_buffer.data(), buffer + header_end + 4, copy_size);
            body_read = copy_size;
        }
        
        // 继续读取剩余的body数据
        while (body_read < content_length) {
            ssize_t bytes_read = recv(client_fd, body_buffer.data() + body_read, 
                                    content_length - body_read, 0);
            if (bytes_read <= 0) {
                break;
            }
            body_read += bytes_read;
        }
        
        // 将body添加到请求中
        raw_request.append(body_buffer.data(), body_read);
    }
    
    HttpRequest request = parse_request(raw_request);
    HttpResponse response;
    
    std::string route_key = request.method + " " + request.path;
    auto route_iter = routes_.find(route_key);
    
    if (route_iter != routes_.end()) {
        route_iter->second(request, response);
    } else {
        if (!handle_static_file(request.path, response)) {
            response.status_code = 404;
            response.body = "Not Found";
        }
    }
    
    std::string response_str = generate_response(response);
    send(client_fd, response_str.c_str(), response_str.length(), 0);
    
    close(client_fd);
}

HttpRequest HttpServer::parse_request(const std::string& raw_request) {
    HttpRequest request;
    
    std::istringstream iss(raw_request);
    std::string line;
    
    if (std::getline(iss, line)) {
        std::istringstream line_iss(line);
        std::string path_query;
        line_iss >> request.method >> path_query;
        
        size_t query_pos = path_query.find('?');
        if (query_pos != std::string::npos) {
            request.path = path_query.substr(0, query_pos);
            request.query_string = path_query.substr(query_pos + 1);
            request.params = parse_query_params(request.query_string);
        } else {
            request.path = path_query;
        }
        
        request.path = url_decode(request.path);
    }
    
    // 解析头部
    bool headers_done = false;
    while (std::getline(iss, line) && !headers_done) {
        // 移除行尾的\r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // 空行表示头部结束
        if (line.empty()) {
            headers_done = true;
            break;
        }
        
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string name = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            value.erase(0, value.find_first_not_of(' '));
            
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            request.headers[name] = value;
        }
    }
    
    // 解析body - 直接从原始请求中提取
    size_t header_end = raw_request.find("\r\n\r\n");
    if (header_end != std::string::npos) {
        request.body = raw_request.substr(header_end + 4);
    }
    
    return request;
}

std::string HttpServer::generate_response(const HttpResponse& response) {
    std::ostringstream oss;
    
    oss << "HTTP/1.1 " << response.status_code;
    switch (response.status_code) {
        case 200: oss << " OK"; break;
        case 400: oss << " Bad Request"; break;
        case 401: oss << " Unauthorized"; break;
        case 403: oss << " Forbidden"; break;
        case 404: oss << " Not Found"; break;
        case 405: oss << " Method Not Allowed"; break;
        case 500: oss << " Internal Server Error"; break;
        default: oss << " Unknown"; break;
    }
    oss << "\r\n";
    
    oss << "Content-Length: " << response.body.length() << "\r\n";
    
    for (const auto& header : response.headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    oss << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    
    oss << "\r\n";
    
    oss << response.body;
    
    return oss.str();
}

bool HttpServer::handle_static_file(const std::string& path, HttpResponse& response) {
    std::string file_path = static_root_;
    
    if (path == "/") {
        file_path += "/index.html";
    } else {
        file_path += path;
    }
    
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    std::ostringstream oss;
    oss << file.rdbuf();
    response.body = oss.str();
    
    std::string ext;
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        ext = file_path.substr(dot_pos + 1);
    }
    
    if (ext == "html") {
        response.headers["Content-Type"] = "text/html; charset=utf-8";
    } else if (ext == "css") {
        response.headers["Content-Type"] = "text/css";
    } else if (ext == "js") {
        response.headers["Content-Type"] = "application/javascript";
    } else if (ext == "json") {
        response.headers["Content-Type"] = "application/json";
    } else if (ext == "png") {
        response.headers["Content-Type"] = "image/png";
    } else if (ext == "jpg" || ext == "jpeg") {
        response.headers["Content-Type"] = "image/jpeg";
    } else {
        response.headers["Content-Type"] = "application/octet-stream";
    }
    
    response.status_code = 200;
    return true;
}

std::string HttpServer::url_decode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

void HttpServer::add_route(const std::string& path, RouteHandler handler) {
    addRoute("GET", path, handler);
}

void HttpServer::add_post_route(const std::string& path, RouteHandler handler) {
    addRoute("POST", path, handler);
}

std::map<std::string, std::string> HttpServer::parse_query_params(const std::string& query_string) {
    std::map<std::string, std::string> params;
    
    if (query_string.empty()) {
        return params;
    }
    
    std::istringstream ss(query_string);
    std::string pair;
    
    while (std::getline(ss, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            
            // URL解码（简化版）
            std::replace(value.begin(), value.end(), '+', ' ');
            params[url_decode(key)] = url_decode(value);
        }
    }
    
    return params;
}

 