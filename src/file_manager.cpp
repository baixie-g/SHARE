#include "file_manager.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

FileManager::FileManager() : storage_root_("shared"), max_file_size_(50 * 1024 * 1024) {
    initializeMimeTypes();
    initializeAllowedTypes();
}

FileManager::~FileManager() {
}

bool FileManager::initialize() {
    if (!createDirectory(storage_root_)) {
        return false;
    }
    
    return createCategoryDirectories();
}

bool FileManager::createCategoryDirectories() {
    std::vector<std::string> categories = {"videos", "documents", "images", "others"};
    
    for (const auto& category : categories) {
        std::string path = storage_root_ + "/" + category;
        if (!createDirectory(path)) {
            return false;
        }
    }
    
    return true;
}

bool FileManager::createDirectory(const std::string& path) {
    try {
        fs::create_directories(path);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory " << path << ": " << e.what() << std::endl;
        return false;
    }
}

bool FileManager::saveUploadedFile(const std::string& content, const std::string& filename, 
                                 const std::string& category, std::string& saved_path) {
    if (!isFileTypeAllowed(filename)) {
        return false;
    }
    
    if (!isFileSizeValid(content.length())) {
        return false;
    }
    
    std::string safe_filename = generateSafeFilename(filename);
    std::string category_dir = storage_root_ + "/" + category;
    
    if (!directoryExists(category_dir)) {
        if (!createDirectory(category_dir)) {
            return false;
        }
    }
    
    saved_path = generateUniqueFilename(category_dir, safe_filename);
    
    try {
        std::ofstream file(saved_path, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        file.write(content.data(), content.length());
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save file: " << e.what() << std::endl;
        return false;
    }
}

std::vector<uint8_t> FileManager::readFile(const std::string& filepath) {
    std::vector<uint8_t> data;
    
    if (!isPathSafe(filepath)) {
        return data;
    }
    
    try {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return data;
        }
        
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        data.resize(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        file.close();
    } catch (const std::exception& e) {
        std::cerr << "Failed to read file: " << e.what() << std::endl;
        data.clear();
    }
    
    return data;
}

long FileManager::getFileSize(const std::string& filepath) {
    if (!isPathSafe(filepath)) {
        return -1;
    }
    
    try {
        if (fs::exists(filepath)) {
            return fs::file_size(filepath);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to get file size: " << e.what() << std::endl;
    }
    
    return -1;
}

std::string FileManager::generatePreview(const std::string& filepath, const std::string& file_type) {
    if (!isPreviewSupported(file_type)) {
        return "";
    }
    
    if (isTextFile(file_type)) {
        return readTextFile(filepath);
    }
    
    return "";
}

bool FileManager::isPreviewSupported(const std::string& file_type) {
    return isTextFile(file_type) || isImageFile(file_type) || isVideoFile(file_type);
}

bool FileManager::isFileTypeAllowed(const std::string& filename) {
    std::string ext = getFileExtension(filename);
    ext = toLowerCase(ext);
    
    return std::find(allowed_types_.begin(), allowed_types_.end(), ext) != allowed_types_.end();
}

bool FileManager::isFileSizeValid(long file_size) {
    return file_size <= max_file_size_;
}

bool FileManager::isPathSafe(const std::string& path) {
    // 检查路径是否包含危险字符
    if (path.find("..") != std::string::npos) {
        return false;
    }
    
    if (path.find("//") != std::string::npos) {
        return false;
    }
    
    // 确保路径在存储根目录下
    try {
        fs::path abs_path = fs::absolute(path);
        fs::path abs_root = fs::absolute(storage_root_);
        
        std::string path_str = abs_path.string();
        std::string root_str = abs_root.string();
        
        return path_str.substr(0, root_str.length()) == root_str;
    } catch (const std::exception& e) {
        return false;
    }
}

std::string FileManager::generateSafeFilename(const std::string& original_filename) {
    std::string safe_name = original_filename;
    
    // 替换危险字符
    std::string dangerous_chars = "\\/:*?\"<>|";
    for (char c : dangerous_chars) {
        std::replace(safe_name.begin(), safe_name.end(), c, '_');
    }
    
    return safe_name;
}

std::string FileManager::determineCategory(const std::string& filename) {
    std::string ext = getFileExtension(filename);
    ext = toLowerCase(ext);
    
    if (ext == ".mp4" || ext == ".avi" || ext == ".mkv" || ext == ".mov" || ext == ".wmv") {
        return "video";
    } else if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" || ext == ".bmp") {
        return "image";
    } else if (ext == ".txt" || ext == ".md" || ext == ".pdf" || ext == ".doc" || ext == ".docx") {
        return "document";
    } else {
        return "other";
    }
}

std::string FileManager::getMimeType(const std::string& filename) {
    std::string ext = getFileExtension(filename);
    ext = toLowerCase(ext);
    
    auto it = mime_types_.find(ext);
    if (it != mime_types_.end()) {
        return it->second;
    }
    
    return "application/octet-stream";
}

bool FileManager::deleteFile(const std::string& filepath) {
    if (!isPathSafe(filepath)) {
        return false;
    }
    
    try {
        return fs::remove(filepath);
    } catch (const std::exception& e) {
        std::cerr << "Failed to delete file: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> FileManager::listFiles(const std::string& directory) {
    std::vector<std::string> files;
    
    if (!isPathSafe(directory)) {
        return files;
    }
    
    try {
        if (fs::exists(directory) && fs::is_directory(directory)) {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path().filename().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to list files: " << e.what() << std::endl;
    }
    
    return files;
}

void FileManager::initializeMimeTypes() {
    mime_types_[".txt"] = "text/plain";
    mime_types_[".md"] = "text/markdown";
    mime_types_[".html"] = "text/html";
    mime_types_[".css"] = "text/css";
    mime_types_[".js"] = "application/javascript";
    mime_types_[".json"] = "application/json";
    mime_types_[".xml"] = "application/xml";
    
    // 图片类型
    mime_types_[".jpg"] = "image/jpeg";
    mime_types_[".jpeg"] = "image/jpeg";
    mime_types_[".png"] = "image/png";
    mime_types_[".gif"] = "image/gif";
    mime_types_[".bmp"] = "image/bmp";
    mime_types_[".svg"] = "image/svg+xml";
    
    // 视频类型
    mime_types_[".mp4"] = "video/mp4";
    mime_types_[".avi"] = "video/x-msvideo";
    mime_types_[".mkv"] = "video/x-matroska";
    mime_types_[".mov"] = "video/quicktime";
    mime_types_[".wmv"] = "video/x-ms-wmv";
    
    // 音频类型
    mime_types_[".mp3"] = "audio/mpeg";
    mime_types_[".wav"] = "audio/wav";
    mime_types_[".ogg"] = "audio/ogg";
    
    // 文档类型
    mime_types_[".pdf"] = "application/pdf";
    mime_types_[".doc"] = "application/msword";
    mime_types_[".docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    mime_types_[".xls"] = "application/vnd.ms-excel";
    mime_types_[".xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    
    // 压缩文件
    mime_types_[".zip"] = "application/zip";
    mime_types_[".rar"] = "application/x-rar-compressed";
    mime_types_[".7z"] = "application/x-7z-compressed";
}

void FileManager::initializeAllowedTypes() {
    allowed_types_ = {
        ".txt", ".md", ".html", ".css", ".js", ".json", ".xml",
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".svg",
        ".mp4", ".avi", ".mkv", ".mov", ".wmv",
        ".mp3", ".wav", ".ogg",
        ".pdf", ".doc", ".docx", ".xls", ".xlsx",
        ".zip", ".rar", ".7z"
    };
}

std::string FileManager::getFileExtension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        return filename.substr(dot_pos);
    }
    return "";
}

std::string FileManager::toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool FileManager::directoryExists(const std::string& path) {
    try {
        return fs::exists(path) && fs::is_directory(path);
    } catch (const std::exception& e) {
        return false;
    }
}

bool FileManager::fileExists(const std::string& path) {
    try {
        return fs::exists(path) && fs::is_regular_file(path);
    } catch (const std::exception& e) {
        return false;
    }
}

std::string FileManager::generateUniqueFilename(const std::string& directory, const std::string& filename) {
    std::string full_path = directory + "/" + filename;
    
    if (!fileExists(full_path)) {
        return full_path;
    }
    
    // 如果文件已存在，添加数字后缀
    std::string name = filename;
    std::string ext = getFileExtension(filename);
    
    if (!ext.empty()) {
        name = filename.substr(0, filename.length() - ext.length());
    }
    
    int counter = 1;
    do {
        std::string new_filename = name + "_" + std::to_string(counter) + ext;
        full_path = directory + "/" + new_filename;
        counter++;
    } while (fileExists(full_path));
    
    return full_path;
}

std::string FileManager::readTextFile(const std::string& filepath) {
    if (!isPathSafe(filepath)) {
        return "";
    }
    
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return "";
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        return content;
    } catch (const std::exception& e) {
        std::cerr << "Failed to read text file: " << e.what() << std::endl;
        return "";
    }
}

bool FileManager::isTextFile(const std::string& file_type) {
    return file_type.find("text/") == 0 || 
           file_type.find("application/json") == 0 ||
           file_type.find("application/xml") == 0;
}

bool FileManager::isVideoFile(const std::string& file_type) {
    return file_type.find("video/") == 0;
}

bool FileManager::isImageFile(const std::string& file_type) {
    return file_type.find("image/") == 0;
} 