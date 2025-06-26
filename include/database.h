#ifndef DATABASE_H
#define DATABASE_H

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
    std::string file_type;
    long file_size;
    std::string upload_time;
    int uploader_id;
    std::string category;  // "video", "document", "image", "other"
    int download_count;
    bool is_public;
};

// 会话信息结构
struct Session {
    std::string session_id;
    int user_id;
    std::string created_at;
    std::string expires_at;
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
    bool createSession(const std::string& session_id, int user_id, int expires_hours = 24);
    
    // 验证会话
    Session* validateSession(const std::string& session_id);
    
    // 删除会话
    bool deleteSession(const std::string& session_id);
    
    // 清理过期会话
    bool cleanExpiredSessions();

    // === 统计功能 ===
    // 获取文件总数
    int getTotalFileCount();
    
    // 获取用户总数
    int getTotalUserCount();
    
    // 获取存储空间使用情况
    long getTotalStorageUsed();

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

#endif // DATABASE_H 