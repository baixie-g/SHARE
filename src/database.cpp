#include "database.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <ctime>
#include <openssl/sha.h>
#include <random>

Database::Database(const std::string& db_path) : db_(nullptr), db_path_(db_path) {
}

Database::~Database() {
    close();
}

bool Database::initialize() {
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    if (!createTables()) {
        std::cerr << "Failed to create tables" << std::endl;
        return false;
    }
    
    if (!createDefaultAdmin()) {
        std::cerr << "Failed to create default admin" << std::endl;
        return false;
    }
    
    std::cout << "数据库初始化成功" << std::endl;
    return true;
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::execute(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

bool Database::createTables() {
    // 创建用户表
    std::string sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            role TEXT NOT NULL DEFAULT 'user',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            active INTEGER DEFAULT 1
        );
    )";
    
    if (!execute(sql)) {
        return false;
    }
    
    // 创建文件表
    sql = R"(
        CREATE TABLE IF NOT EXISTS files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            filename TEXT NOT NULL,
            filepath TEXT NOT NULL,
            file_type TEXT NOT NULL,
            file_size INTEGER NOT NULL,
            upload_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            uploader_id INTEGER,
            category TEXT NOT NULL DEFAULT 'other',
            download_count INTEGER DEFAULT 0,
            is_public INTEGER DEFAULT 1,
            FOREIGN KEY (uploader_id) REFERENCES users (id)
        );
    )";
    
    if (!execute(sql)) {
        return false;
    }
    
    // 创建会话表
    sql = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            session_id TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            expires_at DATETIME NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users (id)
        );
    )";
    
    return execute(sql);
}

bool Database::createDefaultAdmin() {
    // 检查是否已存在admin用户
    sqlite3_stmt* stmt;
    std::string sql = "SELECT COUNT(*) FROM users WHERE username = 'admin'";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    bool adminExists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        adminExists = sqlite3_column_int(stmt, 0) > 0;
    }
    
    sqlite3_finalize(stmt);
    
    if (!adminExists) {
        return createUser("admin", "admin123", "admin");
    }
    
    return true;
}

bool Database::createUser(const std::string& username, const std::string& password, const std::string& role) {
    std::string hashedPassword = hashPassword(password);
    
    sqlite3_stmt* stmt;
    std::string sql = "INSERT INTO users (username, password_hash, role) VALUES (?, ?, ?)";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashedPassword.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_STATIC);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    return success;
}

User* Database::authenticateUser(const std::string& username, const std::string& password) {
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, username, password_hash, role, created_at, active FROM users WHERE username = ? AND active = 1";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return nullptr;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    User* user = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string storedHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        if (verifyPassword(password, storedHash)) {
            user = new User();
            user->id = sqlite3_column_int(stmt, 0);
            user->username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            user->password_hash = storedHash;
            user->role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            user->created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            user->active = sqlite3_column_int(stmt, 5) != 0;
        }
    }
    
    sqlite3_finalize(stmt);
    return user;
}

User* Database::getUserById(int user_id) {
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, username, password_hash, role, created_at, active FROM users WHERE id = ?";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return nullptr;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    User* user = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = new User();
        user->id = sqlite3_column_int(stmt, 0);
        user->username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user->password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user->role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user->created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        user->active = sqlite3_column_int(stmt, 5) != 0;
    }
    
    sqlite3_finalize(stmt);
    return user;
}

User* Database::getUserByUsername(const std::string& username) {
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, username, password_hash, role, created_at, active FROM users WHERE username = ?";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return nullptr;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    User* user = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = new User();
        user->id = sqlite3_column_int(stmt, 0);
        user->username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user->password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user->role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user->created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        user->active = sqlite3_column_int(stmt, 5) != 0;
    }
    
    sqlite3_finalize(stmt);
    return user;
}

std::vector<FileInfo> Database::getPublicFiles(int limit, int offset) {
    std::vector<FileInfo> files;
    
    sqlite3_stmt* stmt;
    std::string sql = "SELECT id, filename, filepath, file_type, file_size, upload_time, uploader_id, category, download_count, is_public FROM files WHERE is_public = 1 ORDER BY upload_time DESC LIMIT ? OFFSET ?";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return files;
    }
    
    sqlite3_bind_int(stmt, 1, limit);
    sqlite3_bind_int(stmt, 2, offset);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileInfo file;
        file.id = sqlite3_column_int(stmt, 0);
        file.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        file.filepath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        file.file_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        file.file_size = sqlite3_column_int64(stmt, 4);
        file.upload_time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        file.uploader_id = sqlite3_column_int(stmt, 6);
        file.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        file.download_count = sqlite3_column_int(stmt, 8);
        file.is_public = sqlite3_column_int(stmt, 9) != 0;
        
        files.push_back(file);
    }
    
    sqlite3_finalize(stmt);
    return files;
}

bool Database::createSession(const std::string& session_id, int user_id, int expires_hours) {
    sqlite3_stmt* stmt;
    std::string sql = "INSERT INTO sessions (session_id, user_id, expires_at) VALUES (?, ?, datetime('now', '+' || ? || ' hours'))";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int(stmt, 3, expires_hours);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    return success;
}

Session* Database::validateSession(const std::string& session_id) {
    sqlite3_stmt* stmt;
    std::string sql = "SELECT session_id, user_id, created_at, expires_at FROM sessions WHERE session_id = ? AND expires_at > datetime('now')";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return nullptr;
    }
    
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    
    Session* session = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        session = new Session();
        session->session_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        session->user_id = sqlite3_column_int(stmt, 1);
        session->created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        session->expires_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    
    sqlite3_finalize(stmt);
    return session;
}

bool Database::deleteSession(const std::string& session_id) {
    sqlite3_stmt* stmt;
    std::string sql = "DELETE FROM sessions WHERE session_id = ?";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    
    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    
    return success;
}

std::string Database::hashPassword(const std::string& password) {
    // 简单的SHA256哈希，实际项目中应该使用更安全的方法如bcrypt
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.length());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

bool Database::verifyPassword(const std::string& password, const std::string& hash) {
    return hashPassword(password) == hash;
}

std::string Database::generateSessionId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::ostringstream oss;
    for (int i = 0; i < 32; ++i) {
        oss << std::hex << dis(gen);
    }
    return oss.str();
}

void Database::cleanupExpiredSessions() {
    const char* sql = "DELETE FROM sessions WHERE expires_at <= datetime('now')";
    sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);
}

int Database::addFileRecord(const std::string& filename, const std::string& filepath,
                          const std::string& category, long size, 
                          const std::string& mime_type, int uploader_id) {
    const char* sql = "INSERT INTO files (filename, filepath, category, size, mime_type, uploader_id) VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, filepath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, category.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, size);
    sqlite3_bind_text(stmt, 5, mime_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, uploader_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return -1;
    }
    
    return sqlite3_last_insert_rowid(db_);
}

std::vector<FileInfo> Database::getFiles(int limit, int offset, const std::string& category) {
    std::string sql = "SELECT id, filename, filepath, category, file_size, file_type, uploader_id, upload_time FROM files";
    
    if (!category.empty()) {
        sql += " WHERE category = ?";
    }
    
    sql += " ORDER BY upload_time DESC LIMIT ? OFFSET ?";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return {};
    }
    
    int param_index = 1;
    if (!category.empty()) {
        sqlite3_bind_text(stmt, param_index++, category.c_str(), -1, SQLITE_STATIC);
    }
    sqlite3_bind_int(stmt, param_index++, limit);
    sqlite3_bind_int(stmt, param_index, offset);
    
    std::vector<FileInfo> files;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileInfo file;
        file.id = sqlite3_column_int(stmt, 0);
        file.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        file.filepath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        file.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        file.size = sqlite3_column_int64(stmt, 4);                    // file_size
        file.mime_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));  // file_type
        file.uploader_id = sqlite3_column_int(stmt, 6);
        file.upload_time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)); // upload_time
        
        // 设置其他字段的默认值
        file.uploader = "User " + std::to_string(file.uploader_id);
        file.download_count = 0;
        file.is_public = true;
        
        files.push_back(file);
    }
    
    sqlite3_finalize(stmt);
    return files;
}

FileInfo* Database::getFileById(int file_id) {
    const char* sql = "SELECT id, filename, filepath, category, file_size, file_type, uploader_id, upload_time FROM files WHERE id = ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return nullptr;
    }
    
    sqlite3_bind_int(stmt, 1, file_id);
    
    FileInfo* file = nullptr;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        file = new FileInfo();
        file->id = sqlite3_column_int(stmt, 0);
        file->filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        file->filepath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        file->category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        file->size = sqlite3_column_int64(stmt, 4);                    // file_size
        file->mime_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));  // file_type
        file->uploader_id = sqlite3_column_int(stmt, 6);
        file->upload_time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)); // upload_time
    }
    
    sqlite3_finalize(stmt);
    return file;
}

FileInfo Database::getFileByName(const std::string& filename) {
    const char* sql = "SELECT id, filename, filepath, category, size, mime_type, uploader_id, uploaded_at FROM files WHERE filename = ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return FileInfo();
    }
    
    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);
    
    FileInfo file;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        file.id = sqlite3_column_int(stmt, 0);
        file.filename = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        file.filepath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        file.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        file.size = sqlite3_column_int64(stmt, 4);
        file.mime_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        file.uploader_id = sqlite3_column_int(stmt, 6);
        file.uploaded_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    }
    
    sqlite3_finalize(stmt);
    return file;
}

int Database::getTotalFileCount(const std::string& category) {
    std::string sql = "SELECT COUNT(*) FROM files";
    
    if (!category.empty()) {
        sql += " WHERE category = ?";
    }
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return 0;
    }
    
    if (!category.empty()) {
        sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_STATIC);
    }
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

int Database::getUserCount() {
    const char* sql = "SELECT COUNT(*) FROM users";
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

int Database::getFileCount() {
    return getTotalFileCount();
}

long Database::getTotalFileSize() {
    const char* sql = "SELECT SUM(size) FROM files";
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

// 为兼容性添加缺失的方法
Session Database::get_session(const std::string& session_id) {
    Session* session = validateSession(session_id);
    if (session) {
        Session result = *session;
        delete session;
        return result;
    }
    return Session();
}

std::vector<FileInfo> Database::get_files(int page, int limit, const std::string& category) {
    int offset = (page - 1) * limit;
    return getFiles(limit, offset, category);
}

int Database::get_total_files(const std::string& category) {
    return getTotalFileCount(category);
}

User Database::get_user(const std::string& username) {
    User* user = getUserByUsername(username);
    if (user) {
        User result = *user;
        delete user;
        return result;
    }
    return User();
}

bool Database::create_session(const std::string& session_id, const std::string& username, const std::string& role) {
    // 首先获取用户ID
    User* user = getUserByUsername(username);
    if (!user) {
        return false;
    }
    
    int user_id = user->id;
    delete user;
    
    // 使用正确的表结构插入会话
    const char* sql = "INSERT INTO sessions (session_id, user_id, expires_at) VALUES (?, ?, datetime('now', '+24 hours'))";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
        return rc == SQLITE_DONE;
}

bool Database::incrementDownloadCount(int file_id) {
    const char* sql = "UPDATE files SET download_count = download_count + 1 WHERE id = ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, file_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<User> Database::getAllUsers() {
    std::vector<User> users;
    const char* sql = "SELECT id, username, password_hash, role, created_at, active FROM users ORDER BY id";
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
        user.active = sqlite3_column_int(stmt, 5) != 0;
        users.push_back(user);
    }
    
    sqlite3_finalize(stmt);
    return users;
}

bool Database::deleteUser(int user_id) {
    const char* sql = "DELETE FROM users WHERE id = ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::deleteFile(int file_id) {
    const char* sql = "DELETE FROM files WHERE id = ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, file_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::create_user(const std::string& username, const std::string& password, const std::string& role) {
    return createUser(username, password, role);
}

int Database::getTotalFileCount() {
    return getTotalFileCount("");
}



bool Database::verify_password(const std::string& username, const std::string& password) {
    // 获取用户的密码哈希
    sqlite3_stmt* stmt;
    std::string sql = "SELECT password_hash FROM users WHERE username = ? AND active = 1";
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    bool result = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* stored_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (stored_hash) {
            // 简单的密码验证：比较哈希值
            std::string input_hash = hashPassword(password);
            result = (input_hash == std::string(stored_hash));
        }
    }
    
    sqlite3_finalize(stmt);
    return result;
}

bool Database::addFile(const std::string& filename, const std::string& filepath, 
                       const std::string& file_type, long file_size, int uploader_id, 
                       const std::string& category, bool is_public) {
    const char* sql = "INSERT INTO files (filename, filepath, file_type, file_size, uploader_id, category, is_public) VALUES (?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, filepath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, file_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, file_size);
    sqlite3_bind_int(stmt, 5, uploader_id);
    sqlite3_bind_text(stmt, 6, category.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, is_public ? 1 : 0);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::string Database::get_session_user(const std::string& session_id) {
    const char* sql = "SELECT u.username FROM sessions s "
                     "JOIN users u ON s.user_id = u.id "
                     "WHERE s.session_id = ? AND s.expires_at > datetime('now')";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return "";
    }
    
    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    
    std::string username;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (name) {
            username = name;
        }
    }
    
    sqlite3_finalize(stmt);
    return username;
} 