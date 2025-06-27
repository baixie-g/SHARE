#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>

// 用户信息结构
struct User {
    int id;
    std::string username;
    std::string password_hash;
    std::string role;  // "admin", "user", "guest"
    std::string created_at;
    bool active;
};

// 文件信息结构
struct FileInfo {
    int id;
    std::string filename;
    std::string filepath;
    std::string mime_type;
    std::string file_type;      // 添加file_type字段
    long size;
    long file_size;             // 添加file_size字段 
    std::string uploader;
    int uploader_id;            // 添加uploader_id字段
    std::string upload_time;
    std::string uploaded_at;    // 添加uploaded_at字段
    std::string category;  // "video", "document", "image", "other"
    int download_count;
    bool is_public;
};

// 会话信息结构
struct Session {
    std::string session_id;
    int user_id;                // 添加user_id字段
    std::string username;
    std::string role;
    std::string created_at;
    std::string expires_at;     // 添加expires_at字段
};

/**
 * 数据库管理类
 * 负责SQLite数据库的所有操作，包括用户管理、文件管理、会话管理等
 */
class Database {
public:
    Database(const std::string& db_path = "g00j_share.db");
    ~Database();

    // 初始化数据库
    bool initialize();
    
    // 关闭数据库连接
    void close();

    // === 用户管理 ===
    // 创建用户
    bool createUser(const std::string& username, const std::string& password, const std::string& role = "user");
    
    // 验证用户登录
    User* authenticateUser(const std::string& username, const std::string& password);
    
    // 根据ID获取用户
    User* getUserById(int user_id);
    
    // 根据用户名获取用户
    User* getUserByUsername(const std::string& username);
    
    // 获取所有用户列表
    std::vector<User> getAllUsers();
    
    // 更新用户信息
    bool updateUser(const User& user);
    
    // 删除用户
    bool deleteUser(int user_id);

    // === 文件管理 ===
    // 添加文件记录
    bool addFile(const std::string& filename, const std::string& filepath, 
                 const std::string& file_type, long file_size, int uploader_id, 
                 const std::string& category = "other", bool is_public = true);
    
    // 获取所有文件列表
    std::vector<FileInfo> getAllFiles(int limit = 100, int offset = 0);
    
    // 获取用户上传的文件
    std::vector<FileInfo> getUserFiles(int user_id, int limit = 100, int offset = 0);
    
    // 获取公开文件列表
    std::vector<FileInfo> getPublicFiles(int limit = 100, int offset = 0);
    
    // 根据分类获取文件
    std::vector<FileInfo> getFilesByCategory(const std::string& category, int limit = 100, int offset = 0);
    
    // 根据ID获取文件信息
    FileInfo* getFileById(int file_id);
    
    // 更新文件下载次数
    bool incrementDownloadCount(int file_id);
    
    // 删除文件记录
    bool deleteFile(int file_id);
    
    // 搜索文件
    std::vector<FileInfo> searchFiles(const std::string& keyword, int limit = 100, int offset = 0);

    // === 会话管理 ===
    // 创建会话
    bool createSession(const std::string& session_id, const std::string& username, const std::string& role);
    bool createSession(const std::string& session_id, int user_id, int expires_hours);  // 重载版本
    
    // 验证会话
    Session* validateSession(const std::string& session_id);
    
    // 删除会话
    bool deleteSession(const std::string& session_id);
    
    // 清理过期会话
    bool cleanExpiredSessions();

    // === 统计功能 ===
    // 获取文件总数
    int getTotalFileCount();
    int getTotalFileCount(const std::string& category);  // 支持按分类统计
    
    // 获取用户总数
    int getTotalUserCount();
    int getUserCount();      // 别名方法
    
    // 获取存储空间使用情况
    long getTotalStorageUsed();
    
    // === 源码中额外需要的方法 ===
    std::string generateSessionId();
    void cleanupExpiredSessions();
    int addFileRecord(const std::string& filename, const std::string& filepath,
                     const std::string& file_type, long file_size, const std::string& uploader, int uploader_id);
    std::vector<FileInfo> getFiles(int limit, int offset, const std::string& category);
    FileInfo getFileByName(const std::string& filename);
    int getFileCount();
    long getTotalFileSize();
    
    // 用于main.cpp的方法别名
    bool verify_password(const std::string& username, const std::string& password);
    User get_user(const std::string& username);
    bool create_session(const std::string& session_id, const std::string& username, const std::string& role);
    Session get_session(const std::string& session_id);
    std::string get_session_user(const std::string& session_id);
    bool create_user(const std::string& username, const std::string& password, const std::string& role = "user");
    std::vector<FileInfo> get_files(int page, int limit, const std::string& category);
    int get_total_files(const std::string& category);

private:
    sqlite3* db_;
    std::string db_path_;
    
    // 执行SQL语句
    bool execute(const std::string& sql);
    
    // 创建表
    bool createTables();
    
    // 创建默认管理员账户
    bool createDefaultAdmin();
    
    // 密码哈希
    std::string hashPassword(const std::string& password);
    
    // 验证密码
    bool verifyPassword(const std::string& password, const std::string& hash);
    
    // 生成时间戳
    std::string getCurrentTimestamp();
    
    // 计算过期时间
    std::string getExpirationTime(int hours);
}; 