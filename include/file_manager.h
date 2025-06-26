#pragma once

#include <string>
#include <vector>
#include <map>
#include "database.h"

// 文件上传结果
struct UploadResult {
    bool success;
    std::string message;
    std::string filename;
    long file_size;
    std::string file_path;
};

/**
 * 文件管理器类
 * 负责文件的上传、下载、预览、安全检查等操作
 */
class FileManager {
private:
    std::string base_path;
    std::map<std::string, std::string> mime_types;
    std::vector<std::string> allowed_types;
    long max_file_size;

public:
    FileManager(const std::string& base_path);
    ~FileManager();
    
    // 初始化
    bool initialize();
    
    // 文件上传
    bool save_file(const std::string& filename, const std::string& content, const std::string& uploader);
    UploadResult upload_file(const std::string& filename, const std::vector<uint8_t>& data);
    
    // 文件读取
    std::string read_file(const std::string& filepath);
    std::string read_text_file(const std::string& filepath);
    std::vector<uint8_t> readFile(const std::string& filepath);
    bool file_exists(const std::string& filepath);
    
    // 文件信息
    std::string get_mime_type(const std::string& filename);
    std::string getMimeType(const std::string& filename);
    std::string get_file_category(const std::string& mime_type);
    std::string determineCategory(const std::string& filename);
    long get_file_size(const std::string& filepath);
    long getFileSize(const std::string& filepath);
    
    // 安全检查
    bool is_safe_path(const std::string& path);
    bool isPathSafe(const std::string& path);
    bool is_allowed_type(const std::string& filename);
    bool isFileTypeAllowed(const std::string& filename);
    bool is_size_valid(size_t size);
    bool isFileSizeValid(long file_size);
    
    // 目录管理
    bool create_directories();
    bool createDirectory(const std::string& path);
    std::string get_category_path(const std::string& category);
    std::vector<std::string> list_files(const std::string& directory);
    std::vector<std::string> listFiles(const std::string& directory);
    
    // 文件保存
    bool saveUploadedFile(const std::string& content, const std::string& filename, 
                         const std::string& category, std::string& saved_path);
    
    // 文件删除
    bool deleteFile(const std::string& filepath);
    
    // 文件预览
    std::string generatePreview(const std::string& filepath, const std::string& file_type);
    bool isPreviewSupported(const std::string& file_type);
    
    // 工具方法
    std::string format_file_size(long size);
    std::string generateSafeFilename(const std::string& original_filename);
    std::map<std::string, std::string> getFileInfo(const std::string& filepath);
    
    // 配置
    void setMaxFileSize(long max_size) { max_file_size = max_size; }
    void setAllowedTypes(const std::vector<std::string>& types) { allowed_types = types; }
    void setStorageRoot(const std::string& root) { base_path = root; }
    long get_max_file_size() const { return max_file_size; }
    
    // 源码中需要的额外方法
    bool is_video_file(const std::string& filename);
    bool is_image_file(const std::string& filename);
    bool is_document_file(const std::string& filename);
    bool is_text_file(const std::string& filename);
    bool create_directory(const std::string& path);
    void set_max_file_size(long size);
    std::string get_file_extension(const std::string& filename);

private:
    void initialize_mime_types();
    void initialize_allowed_types();
    std::string sanitize_filename(const std::string& filename);
    
    // 检查目录是否存在
    bool directoryExists(const std::string& path);
    
    // 检查文件是否存在
    bool fileExists(const std::string& path);
    
    // 生成唯一文件名
    std::string generateUniqueFilename(const std::string& directory, const std::string& filename);
    
    // 读取文本文件内容
    std::string readTextFile(const std::string& filepath);
    
    // 检查是否为文本文件
    bool isTextFile(const std::string& file_type);
    
    // 检查是否为视频文件
    bool isVideoFile(const std::string& file_type);
    
    // 检查是否为图片文件
    bool isImageFile(const std::string& file_type);
}; 