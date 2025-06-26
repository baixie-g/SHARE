#include "database.h"
#include <iostream>
#include <sstream>
#include <random>
#include <iomanip>
#include <functional>
#include <ctime>

Database::Database(const std::string& db_path) : db_(nullptr), db_path_(db_path) {}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::init() {
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    // 创建表
    std::string create_users_table = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            role TEXT DEFAULT 'user',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            active BOOLEAN DEFAULT 1
        )
    )";
    
    std::string create_files_table = R"(
        CREATE TABLE IF NOT EXISTS files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            filename TEXT NOT NULL,
            original_name TEXT NOT NULL,
            file_path TEXT NOT NULL,
            file_type TEXT NOT NULL,
            file_size INTEGER NOT NULL,
            owner_id INTEGER NOT NULL,
            upload_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            is_public BOOLEAN DEFAULT 1,
            category TEXT NOT NULL,
            FOREIGN KEY (owner_id) REFERENCES users (id)
        )
    )";
    
    std::string create_sessions_table = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            session_id TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_access DATETIME DEFAULT CURRENT_TIMESTAMP,
            active BOOLEAN DEFAULT 1,
            FOREIGN KEY (user_id) REFERENCES users (id)
        )
    )";
    
    return execute_sql(create_users_table) && 
           execute_sql(create_files_table) && 
           execute_sql(create_sessions_table);
}

bool Database::execute_sql(const std::string& sql) {
    char* error_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error_msg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << error_msg << std::endl;
        sqlite3_free(error_msg);
        return false;
    }
    
    return true;
}

std::string Database::hash_password(const std::string& password) {
    // 简单的哈希实现，生产环境应使用更安全的方法
    std::hash<std::string> hasher;
    size_t hash_value = hasher(password + "g00j_salt");
    
    std::stringstream ss;
    ss << std::hex << hash_value;
    return ss.str();
}

std::string Database::generate_session_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

std::string Database::get_current_timestamp() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool Database::create_user(const std::string& username, const std::string& password, 
                          const std::string& role) {
    std::string hashed_password = hash_password(password);
    
    const char* sql = "INSERT INTO users (username, password_hash, role) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashed_password.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::unique_ptr<User> Database::get_user(const std::string& username) {
    const char* sql = "SELECT id, username, password_hash, role, created_at, active FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return nullptr;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto user = std::make_unique<User>();
        user->id = sqlite3_column_int(stmt, 0);
        user->username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user->password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user->role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user->created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        user->active = sqlite3_column_int(stmt, 5) == 1;
        
        sqlite3_finalize(stmt);
        return user;
    }
    
    sqlite3_finalize(stmt);
    return nullptr;
}

std::unique_ptr<User> Database::get_user_by_id(int user_id) {
    const char* sql = "SELECT id, username, password_hash, role, created_at, active FROM users WHERE id = ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return nullptr;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto user = std::make_unique<User>();
        user->id = sqlite3_column_int(stmt, 0);
        user->username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user->password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user->role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user->created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        user->active = sqlite3_column_int(stmt, 5) == 1;
        
        sqlite3_finalize(stmt);
        return user;
    }
    
    sqlite3_finalize(stmt);
    return nullptr;
}

bool Database::verify_password(const std::string& username, const std::string& password) {
    auto user = get_user(username);
    if (!user || !user->active) {
        return false;
    }
    
    std::string hashed_password = hash_password(password);
    return user->password_hash == hashed_password;
}

std::vector<User> Database::get_all_users() {
    std::vector<User> users;
    const char* sql = "SELECT id, username, password_hash, role, created_at, active FROM users ORDER BY created_at DESC";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return users;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        user.active = sqlite3_column_int(stmt, 5) == 1;
        
        users.push_back(user);
    }
    
    sqlite3_finalize(stmt);
    return users;
}

std::string Database::create_session(int user_id) {
    std::string session_id = generate_session_id();
    
    const char* sql = "INSERT INTO sessions (session_id, user_id) VALUES (?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return "";
    }
    
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? session_id : "";
}

std::unique_ptr<Session> Database::get_session(const std::string& session_id) {
    const char* sql = "SELECT session_id, user_id, created_at, last_access, active FROM sessions WHERE session_id = ? AND active = 1";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return nullptr;
    }
    
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto session = std::make_unique<Session>();
        session->session_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        session->user_id = sqlite3_column_int(stmt, 1);
        session->created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        session->last_access = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        session->active = sqlite3_column_int(stmt, 4) == 1;
        
        sqlite3_finalize(stmt);
        return session;
    }
    
    sqlite3_finalize(stmt);
    return nullptr;
}

bool Database::update_session_access(const std::string& session_id) {
    const char* sql = "UPDATE sessions SET last_access = CURRENT_TIMESTAMP WHERE session_id = ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::delete_session(const std::string& session_id) {
    const char* sql = "UPDATE sessions SET active = 0 WHERE session_id = ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::add_file(const std::string& filename, const std::string& original_name,
                       const std::string& file_path, const std::string& file_type,
                       long file_size, int owner_id, const std::string& category,
                       bool is_public) {
    const char* sql = R"(
        INSERT INTO files (filename, original_name, file_path, file_type, file_size, owner_id, category, is_public) 
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, original_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, file_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, file_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, file_size);
    sqlite3_bind_int(stmt, 6, owner_id);
    sqlite3_bind_text(stmt, 7, category.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 8, is_public ? 1 : 0);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<FileInfo> Database::get_public_files(int limit, int offset) {
    std::vector<FileInfo> files;
    const char* sql = R"(
        SELECT id, filename, original_name, file_path, file_type, file_size, owner_id, upload_time, is_public, category 
        FROM files WHERE is_public = 1 ORDER BY upload_time DESC LIMIT ? OFFSET ?
    )";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return files;
    }
    
    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileInfo file;
        file.id = sqlite3_column_int(stmt, 0);
        file.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        file.original_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        file.file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        file.file_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        file.file_size = sqlite3_column_int64(stmt, 5);
        file.owner_id = sqlite3_column_int(stmt, 6);
        file.upload_time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        file.is_public = sqlite3_column_int(stmt, 8) == 1;
        file.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        
        files.push_back(file);
    }
    
    sqlite3_finalize(stmt);
    return files;
}

int Database::get_user_count() {
    const char* sql = "SELECT COUNT(*) FROM users WHERE active = 1";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return 0;
    }
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

int Database::get_file_count() {
    const char* sql = "SELECT COUNT(*) FROM files WHERE is_public = 1";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return 0;
    }
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

long Database::get_total_file_size() {
    const char* sql = "SELECT SUM(file_size) FROM files WHERE is_public = 1";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return 0;
    }
    
    long total_size = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total_size = sqlite3_column_int64(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return total_size;
} 