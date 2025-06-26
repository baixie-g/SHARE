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
    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    std::string raw_request(buffer);
    
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
    
    while (std::getline(iss, line) && line != "\r") {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string name = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            value.erase(0, value.find_first_not_of(' '));
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            request.headers[name] = value;
        }
    }
    
    std::string body;
    while (std::getline(iss, line)) {
        body += line + "\n";
    }
    if (!body.empty() && body.back() == '\n') {
        body.pop_back();
    }
    request.body = body;
    
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

 