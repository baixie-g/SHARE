#include "file_manager.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sys/stat.h>
#include <regex>
#include <random>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

FileManager::FileManager(const std::string& base_path) 
    : base_path(base_path), max_file_size(50 * 1024 * 1024) {
    initialize_mime_types();
    initialize_allowed_types();
}

FileManager::~FileManager() {
}

bool FileManager::initialize() {
    // 创建必要的目录
    if (!create_directories()) {
        std::cerr << "创建目录失败" << std::endl;
        return false;
    }
    
    std::cout << "文件管理器初始化成功" << std::endl;
    return true;
}

bool FileManager::save_file(const std::string& filename, const std::string& content, const std::string& uploader) {
    if (!is_allowed_type(filename) || !is_size_valid(content.size())) {
        return false;
    }
    
    std::string safe_filename = sanitize_filename(filename);
    std::string mime_type = get_mime_type(safe_filename);
    std::string category = get_file_category(mime_type);
    std::string category_path = get_category_path(category);
    
    // 确保目录存在
    std::filesystem::create_directories(category_path);
    
    std::string full_path = category_path + "/" + safe_filename;
    
    // 检查文件是否已存在，如果存在则重命名
    int counter = 1;
    std::string final_path = full_path;
    while (std::filesystem::exists(final_path)) {
        size_t dot_pos = safe_filename.find_last_of('.');
        std::string name = safe_filename.substr(0, dot_pos);
        std::string ext = safe_filename.substr(dot_pos);
        final_path = category_path + "/" + name + "_" + std::to_string(counter) + ext;
        counter++;
    }
    
    std::ofstream file(final_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(content.c_str(), content.size());
    file.close();
    
    return true;
}

std::string FileManager::read_file(const std::string& filepath) {
    if (!is_safe_path(filepath) || !file_exists(filepath)) {
        return "";
    }
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

bool FileManager::file_exists(const std::string& filepath) {
    return std::filesystem::exists(filepath);
}

std::string FileManager::get_mime_type(const std::string& filename) {
    // 获取文件扩展名
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = filename.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto it = mime_types.find(ext);
    if (it != mime_types.end()) {
        return it->second;
    }
    
    return "application/octet-stream";
}

std::string FileManager::get_file_category(const std::string& mime_type) {
    if (mime_type.find("video/") == 0) return "videos";
    if (mime_type.find("image/") == 0) return "images";
    if (mime_type.find("text/") == 0 || mime_type == "application/pdf") return "documents";
    return "others";
}

long FileManager::get_file_size(const std::string& filepath) {
    try {
        return std::filesystem::file_size(filepath);
    } catch (const std::exception&) {
        return -1;
    }
}

bool FileManager::is_safe_path(const std::string& path) {
    // 检查路径是否包含危险字符
    if (path.find("..") != std::string::npos ||
        path.find("//") != std::string::npos ||
        path.find("\\") != std::string::npos) {
        return false;
    }
    
    // 确保路径在base_path内
    std::filesystem::path abs_path = std::filesystem::absolute(path);
    std::filesystem::path abs_base = std::filesystem::absolute(base_path);
    
    auto rel_path = std::filesystem::relative(abs_path, abs_base);
    return !rel_path.empty() && rel_path.native()[0] != '.';
}

bool FileManager::is_allowed_type(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return false;
    }
    
    std::string ext = filename.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return std::find(allowed_types.begin(), allowed_types.end(), ext) != allowed_types.end();
}

bool FileManager::is_size_valid(size_t size) {
    return size <= max_file_size;
}

bool FileManager::create_directories() {
    try {
        std::filesystem::create_directories(base_path);
        std::filesystem::create_directories(base_path + "/videos");
        std::filesystem::create_directories(base_path + "/images");
        std::filesystem::create_directories(base_path + "/documents");
        std::filesystem::create_directories(base_path + "/others");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "创建目录失败: " << e.what() << std::endl;
        return false;
    }
}

std::string FileManager::get_category_path(const std::string& category) {
    return base_path + "/" + category;
}

void FileManager::initialize_mime_types() {
    // 视频类型
    mime_types["mp4"] = "video/mp4";
    mime_types["avi"] = "video/x-msvideo";
    mime_types["mkv"] = "video/x-matroska";
    mime_types["mov"] = "video/quicktime";
    mime_types["wmv"] = "video/x-ms-wmv";
    mime_types["flv"] = "video/x-flv";
    
    // 图片类型
    mime_types["jpg"] = "image/jpeg";
    mime_types["jpeg"] = "image/jpeg";
    mime_types["png"] = "image/png";
    mime_types["gif"] = "image/gif";
    mime_types["bmp"] = "image/bmp";
    mime_types["webp"] = "image/webp";
    
    // 文档类型
    mime_types["txt"] = "text/plain";
    mime_types["md"] = "text/markdown";
    mime_types["pdf"] = "application/pdf";
    mime_types["doc"] = "application/msword";
    mime_types["docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    mime_types["xls"] = "application/vnd.ms-excel";
    mime_types["xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    
    // 压缩文件
    mime_types["zip"] = "application/zip";
    mime_types["rar"] = "application/x-rar-compressed";
    mime_types["7z"] = "application/x-7z-compressed";
    
    // 音频文件
    mime_types["mp3"] = "audio/mpeg";
    mime_types["wav"] = "audio/wav";
    mime_types["flac"] = "audio/flac";
}

void FileManager::initialize_allowed_types() {
    allowed_types = {
        // 视频
        "mp4", "avi", "mkv", "mov", "wmv", "flv",
        // 图片
        "jpg", "jpeg", "png", "gif", "bmp", "webp",
        // 文档
        "txt", "md", "pdf", "doc", "docx", "xls", "xlsx",
        // 压缩文件
        "zip", "rar", "7z",
        // 音频
        "mp3", "wav", "flac"
    };
}

std::string FileManager::sanitize_filename(const std::string& filename) {
    std::string result = filename;
    
    // 移除或替换危险字符
    std::string dangerous_chars = "<>:\"/\\|?*";
    for (char c : dangerous_chars) {
        std::replace(result.begin(), result.end(), c, '_');
    }
    
    // 移除控制字符
    result.erase(std::remove_if(result.begin(), result.end(),
        [](char c) { return c >= 0 && c <= 31; }), result.end());
    
    // 限制文件名长度
    if (result.length() > 255) {
        size_t dot_pos = result.find_last_of('.');
        if (dot_pos != std::string::npos && dot_pos > 200) {
            std::string ext = result.substr(dot_pos);
            result = result.substr(0, 255 - ext.length()) + ext;
        } else {
            result = result.substr(0, 255);
        }
    }
    
    return result;
}

bool FileManager::is_video_file(const std::string& filename) {
    std::string ext = get_file_extension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return ext == ".mp4" || ext == ".avi" || ext == ".mkv" || 
           ext == ".mov" || ext == ".wmv" || ext == ".flv" || 
           ext == ".webm";
}

bool FileManager::is_image_file(const std::string& filename) {
    std::string ext = get_file_extension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || 
           ext == ".gif" || ext == ".bmp" || ext == ".webp" || 
           ext == ".svg";
}

bool FileManager::is_document_file(const std::string& filename) {
    std::string ext = get_file_extension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return ext == ".pdf" || ext == ".doc" || ext == ".docx" || 
           ext == ".xls" || ext == ".xlsx" || ext == ".ppt" || 
           ext == ".pptx";
}

bool FileManager::is_text_file(const std::string& filename) {
    std::string ext = get_file_extension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return ext == ".txt" || ext == ".md" || ext == ".json" || 
           ext == ".xml" || ext == ".html" || ext == ".css" || 
           ext == ".js";
}

std::string FileManager::read_text_file(const std::string& filepath) {
    std::string full_path;
    if (filepath.find(base_path) == 0) {
        full_path = filepath;
    } else {
        full_path = base_path + "/" + filepath;
    }
    
    if (!is_safe_path(filepath)) {
        return "";
    }
    
    std::ifstream file(full_path);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

std::string FileManager::format_file_size(long size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size_double = static_cast<double>(size);
    
    while (size_double >= 1024.0 && unit_index < 4) {
        size_double /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size_double << " " << units[unit_index];
    return oss.str();
}

std::vector<std::string> FileManager::list_files(const std::string& directory) {
    // 简单实现，实际项目中可能需要更复杂的目录遍历
    std::vector<std::string> files;
    return files;
}

bool FileManager::create_directory(const std::string& path) {
    struct stat st = {0};
    
    if (stat(path.c_str(), &st) == -1) {
        return mkdir(path.c_str(), 0755) == 0;
    }
    
    return true; // 目录已存在
}

void FileManager::set_max_file_size(long size) {
    max_file_size = size;
}

std::string FileManager::get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    return filename.substr(dot_pos);
} 