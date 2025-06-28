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
#include <cctype>
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

// 通用的用户验证函数
int get_user_id_from_session(const std::map<std::string, std::string>& params) {
    auto cookie_it = params.find("Cookie");
    if (cookie_it == params.end()) {
        cookie_it = params.find("cookie");
    }
    
    if (cookie_it == params.end()) {
        return -1;
    }
    
    std::string session_id = get_session_from_cookies(cookie_it->second);
    if (session_id.empty()) {
        return -1;
    }
    
    Session session = g_database->get_session(session_id);
    if (session.username.empty()) {
        return -1;
    }
    
    User user = g_database->get_user(session.username);
    return user.id;
}

// 从请求中获取用户ID（同时支持headers和params）
int get_user_id_from_request(const HttpRequest& request) {
    // 先尝试从headers获取Cookie
    auto cookie_it = request.headers.find("Cookie");
    if (cookie_it == request.headers.end()) {
        cookie_it = request.headers.find("cookie");
    }
    
    if (cookie_it == request.headers.end()) {
        return -1;
    }
    
    std::string session_id = get_session_from_cookies(cookie_it->second);
    if (session_id.empty()) {
        return -1;
    }
    
    Session session = g_database->get_session(session_id);
    if (session.username.empty()) {
        return -1;
    }
    
    User user = g_database->get_user(session.username);
    return user.id;
}

// 前向声明
std::string handle_login(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_register(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_logout(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_user_profile(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_get_files(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_upload(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_system_status(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_processes(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_get_users(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_delete_user(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_delete_file(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_kill_process(const std::string& body, const std::map<std::string, std::string>& params);

// 新的个人文件管理API
std::string handle_my_files(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_shared_files(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_toggle_share(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_user_storage(const std::string& body, const std::map<std::string, std::string>& params);
std::string handle_admin_files(const std::string& body, const std::map<std::string, std::string>& params);

// 管理员获取所有文件（包含完整信息）
std::string handle_admin_files(const std::string& body, const std::map<std::string, std::string>& params) {
    // 验证管理员权限
    int user_id = get_user_id_from_session(params);
    if (user_id == -1) {
        return JsonHelper::error_response("Authentication required");
    }
    
    // 获取用户信息验证是否为管理员
    User* user = g_database->getUserById(user_id);
    if (!user || user->role != "admin") {
        if (user) delete user;
        return JsonHelper::error_response("Admin permission required");
    }
    delete user;
    
    // 获取所有文件（包含用户信息和分享状态）
    std::vector<FileInfo> files = g_database->getAllFilesForAdmin();
    std::string files_json = JsonHelper::serialize_files(files);
    
    return JsonHelper::data_response(files_json, "Admin files retrieved successfully");
}

// 包装函数：将旧的路由处理器适配为新的签名
void handle_login_route(const HttpRequest& request, HttpResponse& response) {
    // 解析表单数据获取用户名密码
    auto form_data = JsonHelper::parse_form_data(request.body);
    std::string username = form_data["username"];
    std::string password = form_data["password"];
    
    if (username.empty() || password.empty()) {
        response.body = JsonHelper::error_response("Username and password are required");
        response.headers["Content-Type"] = "application/json";
        return;
    }
    
    try {
        bool password_valid = g_database->verify_password(username, password);
        
        if (password_valid) {
            User user = g_database->get_user(username);
            std::string session_id = generate_session_id();
            
            if (g_database->create_session(session_id, username, user.role)) {
                response.body = JsonHelper::success_response("Login successful");
                response.headers["Content-Type"] = "application/json";
                response.headers["Set-Cookie"] = "session_id=" + session_id + "; Path=/; Max-Age=86400";
            } else {
                response.body = JsonHelper::error_response("Session creation failed");
                response.headers["Content-Type"] = "application/json";
            }
        } else {
            response.body = JsonHelper::error_response("Invalid username or password");
            response.headers["Content-Type"] = "application/json";
        }
    } catch (const std::exception& e) {
        response.body = JsonHelper::error_response("Internal server error");
        response.headers["Content-Type"] = "application/json";
    }
}

void handle_register_route(const HttpRequest& request, HttpResponse& response) {
    std::string result = handle_register(request.body, request.params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_user_profile_route(const HttpRequest& request, HttpResponse& response);

void handle_logout_route(const HttpRequest& request, HttpResponse& response) {
    // 将headers和params合并传递给handle_logout
    std::map<std::string, std::string> combined_params = request.params;
    for (const auto& header : request.headers) {
        combined_params[header.first] = header.second;
    }
    
    std::string result = handle_logout(request.body, combined_params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
    
    // 清除Cookie
    response.headers["Set-Cookie"] = "session_id=; Path=/; Max-Age=0; HttpOnly";
}

void handle_user_profile_route(const HttpRequest& request, HttpResponse& response) {
    std::string result = handle_user_profile(request.body, request.headers);
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
    try {
        // 从Cookie中获取session_id
        auto cookie_it = params.find("Cookie");
        if (cookie_it == params.end()) {
            cookie_it = params.find("cookie");
        }
        
        if (cookie_it != params.end()) {
            std::string session_id = get_session_from_cookies(cookie_it->second);
            if (!session_id.empty()) {
                // 从数据库中删除session
                g_database->deleteSession(session_id);
            }
        }
        
        return JsonHelper::success_response("Logout successful");
        
    } catch (const std::exception& e) {
        return JsonHelper::error_response("Logout failed");
    }
}

// 获取用户资料（验证session）
std::string handle_user_profile(const std::string& body, const std::map<std::string, std::string>& params) {
    // 从请求头中获取cookie
    auto cookie_it = params.find("Cookie");
    if (cookie_it == params.end()) {
        cookie_it = params.find("cookie");
    }
    
    if (cookie_it == params.end()) {
        return JsonHelper::error_response("No session found");
    }
    
    std::string session_id = get_session_from_cookies(cookie_it->second);
    if (session_id.empty()) {
        return JsonHelper::error_response("Invalid session");
    }
    
    // 验证session并获取用户信息
    std::string username = g_database->get_session_user(session_id);
    if (username.empty()) {
        return JsonHelper::error_response("Session expired");
    }
    
    User user = g_database->get_user(username);
    if (user.username.empty()) {
        return JsonHelper::error_response("User not found");
    }
    
    // 返回用户信息
    std::string user_json = "{\"username\":\"" + user.username + "\",\"role\":\"" + user.role + "\"}";
    return JsonHelper::data_response(user_json, "User profile retrieved");
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

// 改进的multipart/form-data解析
struct MultipartField {
    std::string content;
    std::string filename;
    std::string content_type;
};

std::map<std::string, MultipartField> parse_multipart_fields_enhanced(const std::string& body, const std::string& boundary) {
    std::map<std::string, MultipartField> fields;
    
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
                
                MultipartField field;
                field.content = content;
                
                // 解析文件名 (如果存在)
                size_t filename_pos = headers.find("filename=\"");
                if (filename_pos != std::string::npos) {
                    filename_pos += 10;
                    size_t filename_end = headers.find("\"", filename_pos);
                    if (filename_end != std::string::npos) {
                        field.filename = headers.substr(filename_pos, filename_end - filename_pos);
                    }
                }
                
                // 解析Content-Type (如果存在)
                size_t content_type_pos = headers.find("Content-Type: ");
                if (content_type_pos != std::string::npos) {
                    content_type_pos += 14;
                    size_t content_type_end = headers.find("\r\n", content_type_pos);
                    if (content_type_end == std::string::npos) {
                        content_type_end = headers.length();
                    }
                    field.content_type = headers.substr(content_type_pos, content_type_end - content_type_pos);
                }
                
                fields[field_name] = field;
            }
        }
        
        pos = content_end;
    }
    
    return fields;
}

// 保持向后兼容的简化版本
std::map<std::string, std::string> parse_multipart_fields(const std::string& body, const std::string& boundary) {
    auto enhanced_fields = parse_multipart_fields_enhanced(body, boundary);
    std::map<std::string, std::string> simple_fields;
    
    for (const auto& field : enhanced_fields) {
        simple_fields[field.first] = field.second.content;
    }
    
    return simple_fields;
}

// 获取文件扩展名
std::string get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos && dot_pos < filename.length() - 1) {
        return filename.substr(dot_pos);
    }
    return "";
}

// 根据文件扩展名获取MIME类型
std::string get_mime_type(const std::string& filename) {
    std::string ext = get_file_extension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".webp") return "image/webp";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".bmp") return "image/bmp";
    if (ext == ".ico") return "image/x-icon";
    
    if (ext == ".mp4") return "video/mp4";
    if (ext == ".avi") return "video/x-msvideo";
    if (ext == ".mov") return "video/quicktime";
    if (ext == ".wmv") return "video/x-ms-wmv";
    if (ext == ".flv") return "video/x-flv";
    if (ext == ".webm") return "video/webm";
    if (ext == ".mkv") return "video/x-matroska";
    
    if (ext == ".mp3") return "audio/mpeg";
    if (ext == ".wav") return "audio/wav";
    if (ext == ".flac") return "audio/flac";
    if (ext == ".aac") return "audio/aac";
    if (ext == ".ogg") return "audio/ogg";
    if (ext == ".wma") return "audio/x-ms-wma";
    
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".doc") return "application/msword";
    if (ext == ".docx") return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (ext == ".xls") return "application/vnd.ms-excel";
    if (ext == ".xlsx") return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if (ext == ".ppt") return "application/vnd.ms-powerpoint";
    if (ext == ".pptx") return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    
    if (ext == ".txt") return "text/plain";
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".xml") return "application/xml";
    if (ext == ".csv") return "text/csv";
    
    if (ext == ".zip") return "application/zip";
    if (ext == ".rar") return "application/vnd.rar";
    if (ext == ".7z") return "application/x-7z-compressed";
    if (ext == ".tar") return "application/x-tar";
    if (ext == ".gz") return "application/gzip";
    
    return "application/octet-stream";
}

// 文件上传
std::string handle_upload(const std::string& body, const std::map<std::string, std::string>& params) {
    try {
        // 验证用户登录
        int user_id = get_user_id_from_session(params);
        if (user_id == -1) {
            return JsonHelper::error_response("Authentication required");
        }
        
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
        
        // 解析multipart数据 (使用增强版本)
        auto enhanced_fields = parse_multipart_fields_enhanced(body, boundary);
        
        auto file_it = enhanced_fields.find("file");
        auto category_it = enhanced_fields.find("category");
        
        if (file_it == enhanced_fields.end()) {
            return JsonHelper::error_response("No file provided");
        }
        
        std::string category = "others";
        if (category_it != enhanced_fields.end()) {
            category = category_it->second.content;
        }
        
        // 获取原始文件名和扩展名
        std::string original_filename = file_it->second.filename;
        if (original_filename.empty()) {
            original_filename = "unnamed_file";
        }
        
        // 生成唯一的文件名，保持原扩展名
        std::string file_extension = get_file_extension(original_filename);
        if (file_extension.empty()) {
            file_extension = ".bin"; // 默认扩展名
        }
        
        std::string filename = "uploaded_" + std::to_string(time(nullptr)) + file_extension;
        std::string filepath = "shared/" + category + "/" + filename;
        
        // 确保目录存在
        std::string dir_path = "shared/" + category;
        mkdir(dir_path.c_str(), 0755);
        
        // 保存文件
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return JsonHelper::error_response("Failed to save file");
        }
        
        file.write(file_it->second.content.c_str(), file_it->second.content.length());
        file.close();
        
        // 检查文件大小和用户配额
        long file_size = file_it->second.content.length();
        auto storage_info = g_database->getUserStorageInfo(user_id);
        long used = storage_info.first;
        long quota = storage_info.second;
        
        if (used + file_size > quota) {
            // 删除已保存的文件
            std::remove(filepath.c_str());
            return JsonHelper::error_response("Storage quota exceeded. Available: " + 
                std::to_string((quota - used) / 1024 / 1024) + "MB");
        }
        
        // 获取正确的MIME类型
        std::string mime_type = get_mime_type(original_filename);
        if (!file_it->second.content_type.empty()) {
            mime_type = file_it->second.content_type; // 如果浏览器提供了MIME类型，优先使用
        }
        
        // 添加到数据库，使用原始文件名和正确的MIME类型
        bool success = g_database->addFile(original_filename, filepath, mime_type, 
                                          file_size, user_id, category, false); // 默认不分享
        
        if (success) {
            // 更新用户存储使用量
            g_database->updateUserStorage(user_id, file_size);
            return JsonHelper::success_response("File uploaded successfully");
        } else {
            // 删除已保存的文件
            std::remove(filepath.c_str());
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
    std::string status_json = JsonHelper::serialize_system_status(status);
    return JsonHelper::data_response(status_json, "System status retrieved");
}

// 进程列表
std::string handle_processes(const std::string& body, const std::map<std::string, std::string>& params) {
    // 检查管理员权限（简化处理）
    auto processes = SystemMonitor::get_processes();
    std::string processes_json = JsonHelper::serialize_processes(processes);
    return JsonHelper::data_response(processes_json, "Processes retrieved");
}

// 文件下载
void handle_download_route(const HttpRequest& request, HttpResponse& response) {
    auto it = request.params.find("id");
    if (it == request.params.end()) {
        response.body = JsonHelper::error_response("Missing file ID");
        response.headers["Content-Type"] = "application/json";
        return;
    }
    
    int file_id = std::stoi(it->second);
    FileInfo* file = g_database->getFileById(file_id);
    
    if (!file) {
        response.body = JsonHelper::error_response("File not found");
        response.headers["Content-Type"] = "application/json";
        delete file;
        return;
    }
    
    // 检查文件是否存在
    std::ifstream filestream(file->filepath, std::ios::binary);
    if (!filestream.is_open()) {
        response.body = JsonHelper::error_response("File not accessible");
        response.headers["Content-Type"] = "application/json";
        delete file;
        return;
    }
    
    // 读取文件内容
    filestream.seekg(0, std::ios::end);
    size_t file_size = filestream.tellg();
    filestream.seekg(0, std::ios::beg);
    
    std::string file_content(file_size, '\0');
    filestream.read(&file_content[0], file_size);
    filestream.close();
    
    // 设置响应头
    response.body = file_content;
    response.headers["Content-Type"] = file->mime_type.empty() ? "application/octet-stream" : file->mime_type;
    response.headers["Content-Disposition"] = "attachment; filename=\"" + file->filename + "\"";
    response.headers["Content-Length"] = std::to_string(file_size);
    
    // 更新下载次数
    g_database->incrementDownloadCount(file_id);
    
    delete file;
}

// 管理员功能 - 获取用户列表
std::string handle_get_users(const std::string& body, const std::map<std::string, std::string>& params) {
    // 简化的权限检查
    std::vector<User> users = g_database->getAllUsers();
    std::string users_json = JsonHelper::serialize_users(users);
    return JsonHelper::data_response(users_json, "Users retrieved successfully");
}

// 管理员功能 - 删除用户
std::string handle_delete_user(const std::string& body, const std::map<std::string, std::string>& params) {
    auto it = params.find("id");
    if (it == params.end()) {
        return JsonHelper::error_response("Missing user ID");
    }
    
    int user_id = std::stoi(it->second);
    if (user_id == 1) {  // 保护管理员账户
        return JsonHelper::error_response("Cannot delete admin user");
    }
    
    if (g_database->deleteUser(user_id)) {
        return JsonHelper::success_response("User deleted successfully");
    } else {
        return JsonHelper::error_response("Failed to delete user");
    }
}

// 管理员功能 - 删除文件
std::string handle_delete_file(const std::string& body, const std::map<std::string, std::string>& params) {
    auto it = params.find("id");
    if (it == params.end()) {
        return JsonHelper::error_response("Missing file ID");
    }
    
    int file_id = std::stoi(it->second);
    FileInfo* file = g_database->getFileById(file_id);
    
    if (!file) {
        return JsonHelper::error_response("File not found");
    }
    
    // 删除物理文件
    if (std::remove(file->filepath.c_str()) != 0) {
        // 文件删除失败，但继续删除数据库记录
    }
    
    bool success = g_database->deleteFile(file_id);
    delete file;
    
    if (success) {
        return JsonHelper::success_response("File deleted successfully");
    } else {
        return JsonHelper::error_response("Failed to delete file from database");
    }
}

// 管理员功能 - 终止进程
std::string handle_kill_process(const std::string& body, const std::map<std::string, std::string>& params) {
    auto it = params.find("pid");
    if (it == params.end()) {
        return JsonHelper::error_response("Missing process PID");
    }
    
    int pid = std::stoi(it->second);
    
    // 保护关键系统进程
    if (pid <= 3) {  // 保护 init, kthreadd 等核心进程
        return JsonHelper::error_response("Cannot kill system process");
    }
    
    if (SystemMonitor::kill_process(pid)) {
        return JsonHelper::success_response("Process terminated successfully");
    } else {
        return JsonHelper::error_response("Failed to terminate process");
    }
}

// === 新的个人文件管理API ===

// 获取我的文件列表
std::string handle_my_files(const std::string& body, const std::map<std::string, std::string>& params) {
    int user_id = get_user_id_from_session(params);
    if (user_id == -1) {
        return JsonHelper::error_response("Authentication required");
    }
    
    int page = 1;
    int limit = 20;
    
    auto it = params.find("page");
    if (it != params.end()) {
        page = std::stoi(it->second);
    }
    
    it = params.find("limit");
    if (it != params.end()) {
        limit = std::stoi(it->second);
    }
    
    std::vector<FileInfo> files = g_database->getUserFiles(user_id, limit, (page - 1) * limit);
    std::string files_json = JsonHelper::serialize_files(files);
    
    // 获取总数（简化版本，这里没有分页总数计算）
    return JsonHelper::data_response(files_json, "My files retrieved successfully");
}


// 获取所有分享的文件
std::string handle_shared_files(const std::string& body, const std::map<std::string, std::string>& params) {
    int page = 1;
    int limit = 20;
    
    auto it = params.find("page");
    if (it != params.end()) {
        page = std::stoi(it->second);
    }
    
    it = params.find("limit");
    if (it != params.end()) {
        limit = std::stoi(it->second);
    }
    
    std::vector<FileInfo> files = g_database->getSharedFiles(limit, (page - 1) * limit);
    std::string files_json = JsonHelper::serialize_files(files);
    
    return JsonHelper::data_response(files_json, "Shared files retrieved successfully");
}

// 切换文件分享状态
std::string handle_toggle_share(const std::string& body, const std::map<std::string, std::string>& params) {
    int user_id = get_user_id_from_session(params);
    if (user_id == -1) {
        return JsonHelper::error_response("Authentication required");
    }
    
    auto form_data = JsonHelper::parse_form_data(body);
    auto it = form_data.find("file_id");
    if (it == form_data.end()) {
        return JsonHelper::error_response("Missing file ID");
    }
    
    int file_id = std::stoi(it->second);
    
    // 验证文件所有权
    FileInfo* file = g_database->getFileById(file_id);
    if (!file) {
        return JsonHelper::error_response("File not found");
    }
    
    if (file->uploader_id != user_id) {
        delete file;
        return JsonHelper::error_response("Permission denied");
    }
    
    bool new_share_status = !file->is_shared;
    delete file;
    
    if (g_database->toggleFileShare(file_id, new_share_status)) {
        std::string message = new_share_status ? "File shared successfully" : "File unshared successfully";
        return JsonHelper::success_response(message);
    } else {
        return JsonHelper::error_response("Failed to toggle share status");
    }
}

// 获取用户存储信息
std::string handle_user_storage(const std::string& body, const std::map<std::string, std::string>& params) {
    int user_id = get_user_id_from_session(params);
    if (user_id == -1) {
        return JsonHelper::error_response("Authentication required");
    }
    
    auto storage_info = g_database->getUserStorageInfo(user_id);
    long used = storage_info.first;
    long quota = storage_info.second;
    
    std::string storage_json = "{\"used\":" + std::to_string(used) + 
                              ",\"quota\":" + std::to_string(quota) + 
                              ",\"available\":" + std::to_string(quota - used) + 
                              ",\"usage_percent\":" + std::to_string((double)used / quota * 100) + "}";
    
    return JsonHelper::data_response(storage_json, "Storage info retrieved successfully");
}

// 包装函数
void handle_get_users_route(const HttpRequest& request, HttpResponse& response) {
    std::string result = handle_get_users(request.body, request.params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_delete_user_route(const HttpRequest& request, HttpResponse& response) {
    // 解析表单数据
    auto form_data = JsonHelper::parse_form_data(request.body);
    std::string result = handle_delete_user(request.body, form_data);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_delete_file_route(const HttpRequest& request, HttpResponse& response) {
    // 解析表单数据
    auto form_data = JsonHelper::parse_form_data(request.body);
    std::string result = handle_delete_file(request.body, form_data);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_kill_process_route(const HttpRequest& request, HttpResponse& response) {
    // 解析表单数据
    auto form_data = JsonHelper::parse_form_data(request.body);
    std::string result = handle_kill_process(request.body, form_data);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

// 新的文件管理路由包装函数
void handle_my_files_route(const HttpRequest& request, HttpResponse& response) {
    // 将headers和params合并
    std::map<std::string, std::string> combined_params = request.params;
    for (const auto& header : request.headers) {
        combined_params[header.first] = header.second;
    }
    
    std::string result = handle_my_files(request.body, combined_params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_shared_files_route(const HttpRequest& request, HttpResponse& response) {
    std::string result = handle_shared_files(request.body, request.params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_toggle_share_route(const HttpRequest& request, HttpResponse& response) {
    // 将headers和params合并
    std::map<std::string, std::string> combined_params = request.params;
    for (const auto& header : request.headers) {
        combined_params[header.first] = header.second;
    }
    
    std::string result = handle_toggle_share(request.body, combined_params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_user_storage_route(const HttpRequest& request, HttpResponse& response) {
    // 将headers和params合并
    std::map<std::string, std::string> combined_params = request.params;
    for (const auto& header : request.headers) {
        combined_params[header.first] = header.second;
    }
    
    std::string result = handle_user_storage(request.body, combined_params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

void handle_admin_files_route(const HttpRequest& request, HttpResponse& response) {
    // 将headers和params合并
    std::map<std::string, std::string> combined_params = request.params;
    for (const auto& header : request.headers) {
        combined_params[header.first] = header.second;
    }
    
    std::string result = handle_admin_files(request.body, combined_params);
    response.body = result;
    response.headers["Content-Type"] = "application/json";
}

int main() {
    std::cout << "启动 112小站 文件共享系统..." << std::endl;
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化数据库
    g_database = new Database("bin/112_share.db");
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
    g_server = new HttpServer(80);
    
    // 设置静态文件目录
    g_server->setStaticRoot("static");
    
    // 注册API路由
    g_server->add_post_route("/api/login", handle_login_route);
    g_server->add_post_route("/api/register", handle_register_route);
    g_server->add_post_route("/api/logout", handle_logout_route);
    g_server->add_route("/api/user/profile", handle_user_profile_route);
    g_server->add_post_route("/api/upload", handle_upload_route);
    
    g_server->add_route("/api/files", handle_get_files_route);
    g_server->add_route("/api/download", handle_download_route);
    g_server->add_route("/api/system/status", handle_system_status_route);
    g_server->add_route("/api/system/processes", handle_processes_route);
    
    // 管理员API
    g_server->add_route("/api/admin/users", handle_get_users_route);
    g_server->add_post_route("/api/admin/delete-user", handle_delete_user_route);
    g_server->add_post_route("/api/admin/delete-file", handle_delete_file_route);
    g_server->add_post_route("/api/admin/kill-process", handle_kill_process_route);
    
    // 新的个人文件管理API
    g_server->add_route("/api/my-files", handle_my_files_route);
    g_server->add_route("/api/shared-files", handle_shared_files_route);
    g_server->add_post_route("/api/toggle-share", handle_toggle_share_route);
    g_server->add_route("/api/user/storage", handle_user_storage_route);
    g_server->add_route("/api/admin/files", handle_admin_files_route);
    
    std::cout << "服务器启动成功，访问地址: http://localhost:80" << std::endl;
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