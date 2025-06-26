#include "file_manager.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

FileManager::FileManager(const std::string& base_path) : base_path_(base_path) {
    init_mime_types();
    init_allowed_extensions();
    
    // 创建基础目录
    create_directory(base_path_);
    create_directory(base_path_ + "/videos");
    create_directory(base_path_ + "/articles");
    create_directory(base_path_ + "/images");
    create_directory(base_path_ + "/documents");
    create_directory(base_path_ + "/archives");
}

void FileManager::init_mime_types() {
    mime_types_ = {
        // 图片
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".svg", "image/svg+xml"},
        {".webp", "image/webp"},
        
        // 视频
        {".mp4", "video/mp4"},
        {".avi", "video/x-msvideo"},
        {".mkv", "video/x-matroska"},
        {".mov", "video/quicktime"},
        {".wmv", "video/x-ms-wmv"},
        {".flv", "video/x-flv"},
        {".webm", "video/webm"},
        
        // 文档
        {".txt", "text/plain"},
        {".md", "text/markdown"},
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".pdf", "application/pdf"},
        {".doc", "application/msword"},
        {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {".xls", "application/vnd.ms-excel"},
        {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {".ppt", "application/vnd.ms-powerpoint"},
        {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
        
        // 压缩文件
        {".zip", "application/zip"},
        {".rar", "application/x-rar-compressed"},
        {".7z", "application/x-7z-compressed"},
        {".tar", "application/x-tar"},
        {".gz", "application/gzip"},
        {".bz2", "application/x-bzip2"},
        
        // 音频
        {".mp3", "audio/mpeg"},
        {".wav", "audio/wav"},
        {".flac", "audio/flac"},
        {".ogg", "audio/ogg"}
    };
}

void FileManager::init_allowed_extensions() {
    allowed_extensions_ = {
        // 图片
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".svg", ".webp",
        
        // 视频
        ".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm",
        
        // 文档
        ".txt", ".md", ".html", ".css", ".js", ".json", ".xml", ".pdf",
        ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
        
        // 压缩文件
        ".zip", ".rar", ".7z", ".tar", ".gz", ".bz2",
        
        // 音频
        ".mp3", ".wav", ".flac", ".ogg"
    };
}

std::string FileManager::save_uploaded_file(const std::string& content, 
                                           const std::string& original_filename,
                                           FileType type) {
    if (!validate_filename(original_filename)) {
        return "";
    }
    
    if (!check_file_size_limit(content.length())) {
        return "";
    }
    
    std::string category_dir = get_file_category(type);
    std::string target_dir = base_path_ + "/" + category_dir;
    
    if (!create_directory(target_dir)) {
        return "";
    }
    
    std::string sanitized_name = sanitize_filename(original_filename);
    std::string unique_filename = generate_unique_filename(target_dir, sanitized_name);
    std::string full_path = target_dir + "/" + unique_filename;
    
    std::ofstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    file.write(content.c_str(), content.length());
    file.close();
    
    // 返回相对路径
    return category_dir + "/" + unique_filename;
}

bool FileManager::delete_file(const std::string& file_path) {
    if (!is_safe_path(file_path)) {
        return false;
    }
    
    std::string full_path = base_path_ + "/" + file_path;
    try {
        return fs::remove(full_path);
    } catch (const std::exception& e) {
        std::cerr << "Error deleting file: " << e.what() << std::endl;
        return false;
    }
}

bool FileManager::file_exists(const std::string& file_path) {
    if (!is_safe_path(file_path)) {
        return false;
    }
    
    std::string full_path = base_path_ + "/" + file_path;
    return fs::exists(full_path);
}

long FileManager::get_file_size(const std::string& file_path) {
    if (!is_safe_path(file_path)) {
        return -1;
    }
    
    std::string full_path = base_path_ + "/" + file_path;
    try {
        return fs::file_size(full_path);
    } catch (const std::exception& e) {
        return -1;
    }
}

std::string FileManager::get_file_content(const std::string& file_path) {
    if (!is_safe_path(file_path)) {
        return "";
    }
    
    std::string full_path = base_path_ + "/" + file_path;
    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

bool FileManager::is_safe_path(const std::string& path) {
    // 检查路径遍历攻击
    if (path.find("..") != std::string::npos ||
        path.find("//") != std::string::npos ||
        path.find("\\") != std::string::npos ||
        path[0] == '/' ||
        path[0] == '\\') {
        return false;
    }
    
    // 检查是否在允许的基础路径下
    std::string full_path = base_path_ + "/" + path;
    try {
        std::string canonical_base = fs::canonical(base_path_).string();
        std::string canonical_path = fs::weakly_canonical(full_path).string();
        
        return canonical_path.find(canonical_base) == 0;
    } catch (const std::exception& e) {
        return false;
    }
}

FileType FileManager::detect_file_type(const std::string& filename) {
    std::string extension = get_file_extension(filename);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == ".mp4" || extension == ".avi" || extension == ".mkv" || 
        extension == ".mov" || extension == ".wmv" || extension == ".flv" || 
        extension == ".webm") {
        return FileType::VIDEO;
    }
    
    if (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || 
        extension == ".gif" || extension == ".bmp" || extension == ".svg" || 
        extension == ".webp") {
        return FileType::IMAGE;
    }
    
    if (extension == ".txt" || extension == ".md" || extension == ".html" || 
        extension == ".css" || extension == ".js" || extension == ".json" || 
        extension == ".xml" || extension == ".pdf" || extension == ".doc" || 
        extension == ".docx" || extension == ".xls" || extension == ".xlsx" || 
        extension == ".ppt" || extension == ".pptx") {
        return FileType::DOCUMENT;
    }
    
    if (extension == ".zip" || extension == ".rar" || extension == ".7z" || 
        extension == ".tar" || extension == ".gz" || extension == ".bz2") {
        return FileType::ARCHIVE;
    }
    
    return FileType::OTHER;
}

std::string FileManager::get_file_category(FileType type) {
    switch (type) {
        case FileType::VIDEO: return "videos";
        case FileType::IMAGE: return "images";
        case FileType::DOCUMENT: return "documents";
        case FileType::ARCHIVE: return "archives";
        default: return "documents";
    }
}

bool FileManager::is_allowed_file_type(const std::string& filename) {
    std::string extension = get_file_extension(filename);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return std::find(allowed_extensions_.begin(), allowed_extensions_.end(), extension) 
           != allowed_extensions_.end();
}

std::string FileManager::get_mime_type(const std::string& filename) {
    std::string extension = get_file_extension(filename);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    auto it = mime_types_.find(extension);
    return (it != mime_types_.end()) ? it->second : "application/octet-stream";
}

std::vector<std::string> FileManager::list_directory(const std::string& dir_path) {
    std::vector<std::string> files;
    
    if (!is_safe_path(dir_path)) {
        return files;
    }
    
    std::string full_path = base_path_ + "/" + dir_path;
    
    try {
        for (const auto& entry : fs::directory_iterator(full_path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().filename().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error listing directory: " << e.what() << std::endl;
    }
    
    return files;
}

bool FileManager::create_directory(const std::string& dir_path) {
    try {
        return fs::create_directories(dir_path);
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        return false;
    }
}

bool FileManager::validate_filename(const std::string& filename) {
    if (filename.empty() || filename.length() > 255) {
        return false;
    }
    
    // 检查非法字符
    const std::string illegal_chars = "<>:\"/\\|?*";
    for (char c : filename) {
        if (illegal_chars.find(c) != std::string::npos || c < 32) {
            return false;
        }
    }
    
    // 检查是否为保留名称（Windows）
    const std::vector<std::string> reserved_names = {
        "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5",
        "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4",
        "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    std::string name_upper = filename;
    std::transform(name_upper.begin(), name_upper.end(), name_upper.begin(), ::toupper);
    
    for (const auto& reserved : reserved_names) {
        if (name_upper == reserved || name_upper.find(reserved + ".") == 0) {
            return false;
        }
    }
    
    return is_allowed_file_type(filename);
}

std::string FileManager::sanitize_filename(const std::string& filename) {
    std::string sanitized = filename;
    
    // 替换非法字符
    const std::string illegal_chars = "<>:\"/\\|?*";
    for (char& c : sanitized) {
        if (illegal_chars.find(c) != std::string::npos || c < 32) {
            c = '_';
        }
    }
    
    // 移除开头和结尾的点和空格
    while (!sanitized.empty() && (sanitized.front() == '.' || sanitized.front() == ' ')) {
        sanitized.erase(0, 1);
    }
    while (!sanitized.empty() && (sanitized.back() == '.' || sanitized.back() == ' ')) {
        sanitized.pop_back();
    }
    
    // 确保不为空
    if (sanitized.empty()) {
        sanitized = "unnamed_file";
    }
    
    return sanitized;
}

bool FileManager::check_file_size_limit(long size, long max_size) {
    return size <= max_size;
}

std::string FileManager::get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos || dot_pos == filename.length() - 1) {
        return "";
    }
    return filename.substr(dot_pos);
}

std::string FileManager::generate_unique_filename(const std::string& directory, 
                                                 const std::string& filename) {
    std::string base_name = filename;
    std::string extension = get_file_extension(filename);
    
    if (!extension.empty()) {
        base_name = filename.substr(0, filename.length() - extension.length());
    }
    
    std::string candidate = filename;
    int counter = 1;
    
    while (fs::exists(directory + "/" + candidate)) {
        candidate = base_name + "_" + std::to_string(counter) + extension;
        counter++;
    }
    
    return candidate;
}

bool FileManager::extract_archive(const std::string& archive_path, const std::string& extract_to) {
    if (!is_safe_path(archive_path) || !is_safe_path(extract_to)) {
        return false;
    }
    
    std::string full_archive_path = base_path_ + "/" + archive_path;
    std::string full_extract_path = base_path_ + "/" + extract_to;
    
    std::string extension = get_file_extension(archive_path);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (!create_directory(full_extract_path)) {
        return false;
    }
    
    std::string command;
    if (extension == ".zip") {
        command = "cd \"" + full_extract_path + "\" && unzip \"" + full_archive_path + "\"";
    } else if (extension == ".tar") {
        command = "cd \"" + full_extract_path + "\" && tar -xf \"" + full_archive_path + "\"";
    } else if (extension == ".gz") {
        command = "cd \"" + full_extract_path + "\" && tar -xzf \"" + full_archive_path + "\"";
    } else if (extension == ".bz2") {
        command = "cd \"" + full_extract_path + "\" && tar -xjf \"" + full_archive_path + "\"";
    } else {
        return false; // 不支持的压缩格式
    }
    
    int result = system(command.c_str());
    return result == 0;
}

bool FileManager::generate_thumbnail(const std::string& image_path, const std::string& thumb_path) {
    // 简单实现，实际项目中可以使用 ImageMagick 或其他库
    if (!is_safe_path(image_path) || !is_safe_path(thumb_path)) {
        return false;
    }
    
    std::string full_image_path = base_path_ + "/" + image_path;
    std::string full_thumb_path = base_path_ + "/" + thumb_path;
    
    // 使用 ImageMagick 生成缩略图
    std::string command = "convert \"" + full_image_path + "\" -resize 200x200> \"" + full_thumb_path + "\"";
    int result = system(command.c_str());
    
    return result == 0;
} 