#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// 文件类型枚举
enum class FileType {
    VIDEO,
    DOCUMENT,
    IMAGE,
    ARCHIVE,
    OTHER
};

// 文件管理器类
class FileManager {
public:
    FileManager(const std::string& base_path = "shared");
    
    // 文件操作
    std::string save_uploaded_file(const std::string& content, 
                                  const std::string& original_filename,
                                  FileType type);
    bool delete_file(const std::string& file_path);
    bool file_exists(const std::string& file_path);
    long get_file_size(const std::string& file_path);
    std::string get_file_content(const std::string& file_path);
    bool is_safe_path(const std::string& path);
    
    // 文件类型检测
    FileType detect_file_type(const std::string& filename);
    std::string get_file_category(FileType type);
    bool is_allowed_file_type(const std::string& filename);
    std::string get_mime_type(const std::string& filename);
    
    // 目录操作
    std::vector<std::string> list_directory(const std::string& dir_path);
    bool create_directory(const std::string& dir_path);
    bool extract_archive(const std::string& archive_path, const std::string& extract_to);
    
    // 安全检查
    bool validate_filename(const std::string& filename);
    std::string sanitize_filename(const std::string& filename);
    bool check_file_size_limit(long size, long max_size = 50 * 1024 * 1024); // 50MB
    
    // 缩略图生成（如果需要）
    bool generate_thumbnail(const std::string& image_path, const std::string& thumb_path);
    
private:
    std::string base_path_;
    std::unordered_map<std::string, std::string> mime_types_;
    std::vector<std::string> allowed_extensions_;
    
    void init_mime_types();
    void init_allowed_extensions();
    std::string get_file_extension(const std::string& filename);
    std::string generate_unique_filename(const std::string& directory, 
                                       const std::string& filename);
}; 