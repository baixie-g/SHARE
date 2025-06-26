#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>
#include <map>

/**
 * 文件管理器类
 * 负责文件的上传、下载、预览、安全检查等操作
 */
class FileManager {
public:
    FileManager();
    ~FileManager();

    // 初始化文件管理器，创建必要的目录
    bool initialize();

    // === 文件上传 ===
    // 保存上传的文件
    bool saveUploadedFile(const std::string& content, const std::string& filename, 
                         const std::string& category, std::string& saved_path);
    
    // 处理分块上传
    bool handleChunkedUpload(const std::string& chunk_data, const std::string& filename,
                           int chunk_index, int total_chunks, std::string& final_path);

    // === 文件下载 ===
    // 读取文件内容
    std::vector<uint8_t> readFile(const std::string& filepath);
    
    // 支持范围请求的文件读取
    std::vector<uint8_t> readFileRange(const std::string& filepath, long start, long end);
    
    // 获取文件大小
    long getFileSize(const std::string& filepath);

    // === 文件预览 ===
    // 生成文件预览内容
    std::string generatePreview(const std::string& filepath, const std::string& file_type);
    
    // 检查文件是否支持在线预览
    bool isPreviewSupported(const std::string& file_type);

    // === 文件安全 ===
    // 验证文件类型是否允许
    bool isFileTypeAllowed(const std::string& filename);
    
    // 检查文件大小是否在限制内
    bool isFileSizeValid(long file_size);
    
    // 安全的路径检查，防止路径遍历攻击
    bool isPathSafe(const std::string& path);
    
    // 生成安全的文件名
    std::string generateSafeFilename(const std::string& original_filename);

    // === 文件压缩 ===
    // 解压文件到指定目录
    bool extractArchive(const std::string& archive_path, const std::string& extract_dir);
    
    // 检查是否为压缩文件
    bool isArchiveFile(const std::string& filename);

    // === 目录管理 ===
    // 创建目录
    bool createDirectory(const std::string& path);
    
    // 删除文件
    bool deleteFile(const std::string& filepath);
    
    // 获取目录下的文件列表
    std::vector<std::string> listFiles(const std::string& directory);
    
    // 获取文件信息
    std::map<std::string, std::string> getFileInfo(const std::string& filepath);

    // === 文件分类 ===
    // 根据文件扩展名确定分类
    std::string determineCategory(const std::string& filename);
    
    // 获取MIME类型
    std::string getMimeType(const std::string& filename);

    // === 配置管理 ===
    // 设置最大文件大小限制
    void setMaxFileSize(long max_size) { max_file_size_ = max_size; }
    
    // 设置允许的文件类型
    void setAllowedTypes(const std::vector<std::string>& types) { allowed_types_ = types; }
    
    // 设置存储根目录
    void setStorageRoot(const std::string& root) { storage_root_ = root; }

private:
    std::string storage_root_;
    long max_file_size_;
    std::vector<std::string> allowed_types_;
    std::map<std::string, std::string> mime_types_;
    
    // 初始化MIME类型映射
    void initializeMimeTypes();
    
    // 初始化允许的文件类型
    void initializeAllowedTypes();
    
    // 创建分类目录
    bool createCategoryDirectories();
    
    // 获取文件扩展名
    std::string getFileExtension(const std::string& filename);
    
    // 转换为小写
    std::string toLowerCase(const std::string& str);
    
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

#endif // FILE_MANAGER_H 