#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include "database.h"

// 前向声明
struct User;
struct FileInfo;

// JSON值类型枚举
enum class JsonType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

// JSON值类
class JsonValue {
private:
    JsonType type;
    std::string string_value;
    double number_value;
    bool boolean_value;
    std::vector<std::shared_ptr<JsonValue>> array_value;
    std::unordered_map<std::string, std::shared_ptr<JsonValue>> object_value;

public:
    JsonValue();
    JsonValue(const std::string& str);
    JsonValue(const char* str);
    JsonValue(int num);
    JsonValue(long num);
    JsonValue(double num);
    JsonValue(bool val);
    
    // 设置为数组
    void set_array();
    
    // 设置为对象
    void set_object();
    
    // 添加数组元素
    void add_array_element(std::shared_ptr<JsonValue> value);
    
    // 设置对象属性
    void set_object_property(const std::string& key, std::shared_ptr<JsonValue> value);
    
    // 序列化为JSON字符串
    std::string to_string() const;
    
    // 获取类型
    JsonType get_type() const { return type; }
    
    // 静态工厂方法
    static std::shared_ptr<JsonValue> create_string(const std::string& str);
    static std::shared_ptr<JsonValue> create_number(double num);
    static std::shared_ptr<JsonValue> create_boolean(bool val);
    static std::shared_ptr<JsonValue> create_null();
    static std::shared_ptr<JsonValue> create_array();
    static std::shared_ptr<JsonValue> create_object();
};

// JSON助手类
class JsonHelper {
public:
    // API响应生成
    static std::string success_response(const std::string& message = "success");
    static std::string error_response(const std::string& message, int code = 400);
    static std::string data_response(const std::string& data, const std::string& message = "success");
    
    // 对象序列化
    static std::string serialize_user(const User& user);
    static std::string serialize_file(const FileInfo& file);
    static std::string serialize_session(const Session& session);
    
    // 数组序列化
    static std::string serialize_users(const std::vector<User>& users);
    static std::string serialize_files(const std::vector<FileInfo>& files);
    
    // 分页响应
    static std::string paginated_response(const std::string& data, int total, int page, int limit);
    
    // 系统状态序列化
    static std::string serialize_system_status(const std::map<std::string, std::string>& status);
    static std::string serialize_processes(const std::vector<std::map<std::string, std::string>>& processes);
    
    // 工具方法
    static std::string escape_json_string(const std::string& str);
    static std::map<std::string, std::string> parse_form_data(const std::string& data);
    
    // JsonValue工厂方法
    static std::shared_ptr<JsonValue> user_to_json(const User& user);
    static std::shared_ptr<JsonValue> file_info_to_json(const FileInfo& file);
    static std::shared_ptr<JsonValue> users_to_json(const std::vector<User>& users);
    static std::shared_ptr<JsonValue> files_to_json(const std::vector<FileInfo>& files);
    
    // 响应生成方法
    static std::string create_success_response(const std::string& message, std::shared_ptr<JsonValue> data = nullptr);
    static std::string create_error_response(const std::string& message, int code = 400);
    static std::string create_paginated_response(const std::vector<FileInfo>& files, int total, int page, int limit);
    static std::string create_system_status_response(double cpu_usage, double memory_usage, double disk_usage, int process_count);
    
    // 其他工具方法
    static std::shared_ptr<JsonValue> params_to_json(const std::unordered_map<std::string, std::string>& params);
    static std::string objectToJson(const std::unordered_map<std::string, std::string>& obj);
    static std::string arrayToJson(const std::vector<std::string>& arr);
    static std::string valueToJson(const JsonValue& value);
    static std::string createSuccessResponse(const std::string& message, const std::unordered_map<std::string, std::string>& data);
    static std::string createErrorResponse(const std::string& error, int code);
    static std::string createFileListResponse(const std::vector<std::map<std::string, std::string>>& files);
    static std::string createUserResponse(const std::string& username, const std::string& role, int id);
    static std::string createSystemStatusResponse(const std::map<std::string, std::string>& status);
    static std::string getString(const JsonValue& value, const std::string& default_val);
    static int getInt(const JsonValue& value, int default_val);
    static double getDouble(const JsonValue& value, double default_val);
    static bool getBool(const JsonValue& value, bool default_val);
    static std::unordered_map<std::string, std::string> parseObject(const std::string& json);
    static bool hasRequiredFields(const std::unordered_map<std::string, std::string>& obj, const std::vector<std::string>& fields);
    static std::string parseString(const std::string& str, size_t& pos);
    static JsonValue parseNumber(const std::string& str, size_t& pos);
    static JsonValue parseLiteral(const std::string& str, size_t& pos);
    static void skipWhitespace(const std::string& str, size_t& pos);
    static std::string escapeString(const std::string& str);
    static bool isNumberStart(char c);
    static bool isAlpha(char c);
    static bool isDigit(char c);
    static bool isWhitespace(char c);
}; 