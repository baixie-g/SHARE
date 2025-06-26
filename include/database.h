#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// 用户结构
struct User {
    int id;
    std::string username;
    std::string password_hash;
    std::string role; // "admin", "user"
    std::string created_at;
    bool active;
};

// 文件信息结构
struct FileInfo {
    int id;
    std::string filename;
    std::string original_name;
    std::string file_path;
    std::string file_type;
    long file_size;
    int owner_id;
    std::string upload_time;
    bool is_public;
    std::string category; // "video", "document", "image", "other"
};

// 会话结构
struct Session {
    std::string session_id;
    int user_id;
    std::string created_at;
    std::string last_access;
    bool active;
};

// 数据库管理类
class Database {
public:
    Database(const std::string& db_path = "g00j_share.db");
    ~Database();
    
    bool init();
    
    // 用户管理
    bool create_user(const std::string& username, const std::string& password, 
                    const std::string& role = "user");
    std::unique_ptr<User> get_user(const std::string& username);
    std::unique_ptr<User> get_user_by_id(int user_id);
    bool verify_password(const std::string& username, const std::string& password);
    std::vector<User> get_all_users();
    bool update_user(int user_id, const std::string& field, const std::string& value);
    bool delete_user(int user_id);
    
    // 会话管理
    std::string create_session(int user_id);
    std::unique_ptr<Session> get_session(const std::string& session_id);
    bool update_session_access(const std::string& session_id);
    bool delete_session(const std::string& session_id);
    bool cleanup_expired_sessions();
    
    // 文件管理
    bool add_file(const std::string& filename, const std::string& original_name,
                 const std::string& file_path, const std::string& file_type,
                 long file_size, int owner_id, const std::string& category,
                 bool is_public = true);
    std::vector<FileInfo> get_files(int limit = 50, int offset = 0);
    std::vector<FileInfo> get_user_files(int user_id, int limit = 50, int offset = 0);
    std::vector<FileInfo> get_public_files(int limit = 50, int offset = 0);
    std::unique_ptr<FileInfo> get_file_by_id(int file_id);
    std::unique_ptr<FileInfo> get_file_by_path(const std::string& file_path);
    bool delete_file(int file_id);
    bool update_file_publicity(int file_id, bool is_public);
    
    // 统计信息
    int get_user_count();
    int get_file_count();
    long get_total_file_size();
    
private:
    sqlite3* db_;
    std::string db_path_;
    
    bool execute_sql(const std::string& sql);
    std::string hash_password(const std::string& password);
    std::string generate_session_id();
    std::string get_current_timestamp();
}; 