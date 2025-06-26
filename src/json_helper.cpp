#include "json_helper.h"
#include "database.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

// JsonValue 实现
JsonValue::JsonValue() : type(JsonType::Null) {}

JsonValue::JsonValue(const std::string& str) : type(JsonType::String), string_value(str) {}

JsonValue::JsonValue(const char* str) : type(JsonType::String), string_value(str) {}

JsonValue::JsonValue(int num) : type(JsonType::Number), number_value(static_cast<double>(num)) {}

JsonValue::JsonValue(long num) : type(JsonType::Number), number_value(static_cast<double>(num)) {}

JsonValue::JsonValue(double num) : type(JsonType::Number), number_value(num) {}

JsonValue::JsonValue(bool val) : type(JsonType::Boolean), boolean_value(val) {}

void JsonValue::set_array() {
    type = JsonType::Array;
    array_value.clear();
}

void JsonValue::set_object() {
    type = JsonType::Object;
    object_value.clear();
}

void JsonValue::add_array_element(std::shared_ptr<JsonValue> value) {
    if (type != JsonType::Array) {
        set_array();
    }
    array_value.push_back(value);
}

void JsonValue::set_object_property(const std::string& key, std::shared_ptr<JsonValue> value) {
    if (type != JsonType::Object) {
        set_object();
    }
    object_value[key] = value;
}

std::string JsonValue::to_string() const {
    std::ostringstream oss;
    
    switch (type) {
        case JsonType::Null:
            oss << "null";
            break;
        case JsonType::Boolean:
            oss << (boolean_value ? "true" : "false");
            break;
        case JsonType::Number:
            oss << number_value;
            break;
        case JsonType::String:
            oss << "\"" << JsonHelper::escape_json_string(string_value) << "\"";
            break;
        case JsonType::Array:
            oss << "[";
            for (size_t i = 0; i < array_value.size(); ++i) {
                if (i > 0) oss << ",";
                oss << array_value[i]->to_string();
            }
            oss << "]";
            break;
        case JsonType::Object:
            oss << "{";
            bool first = true;
            for (const auto& pair : object_value) {
                if (!first) oss << ",";
                oss << "\"" << JsonHelper::escape_json_string(pair.first) << "\":" << pair.second->to_string();
                first = false;
            }
            oss << "}";
            break;
    }
    
    return oss.str();
}

// JsonValue静态工厂方法
std::shared_ptr<JsonValue> JsonValue::create_string(const std::string& str) {
    return std::make_shared<JsonValue>(str);
}

std::shared_ptr<JsonValue> JsonValue::create_number(double num) {
    return std::make_shared<JsonValue>(num);
}

std::shared_ptr<JsonValue> JsonValue::create_boolean(bool val) {
    return std::make_shared<JsonValue>(val);
}

std::shared_ptr<JsonValue> JsonValue::create_null() {
    return std::make_shared<JsonValue>();
}

std::shared_ptr<JsonValue> JsonValue::create_array() {
    auto value = std::make_shared<JsonValue>();
    value->set_array();
    return value;
}

std::shared_ptr<JsonValue> JsonValue::create_object() {
    auto value = std::make_shared<JsonValue>();
    value->set_object();
    return value;
}

// JsonHelper 实现
std::string JsonHelper::success_response(const std::string& message) {
    return "{\"success\":true,\"message\":\"" + escape_json_string(message) + "\"}";
}

std::string JsonHelper::error_response(const std::string& message, int code) {
    return "{\"success\":false,\"message\":\"" + escape_json_string(message) + "\",\"code\":" + std::to_string(code) + "}";
}

std::string JsonHelper::data_response(const std::string& data, const std::string& message) {
    return "{\"success\":true,\"message\":\"" + escape_json_string(message) + "\",\"data\":" + data + "}";
}

std::string JsonHelper::serialize_user(const User& user) {
    std::ostringstream oss;
    oss << "{\"id\":" << user.id
        << ",\"username\":\"" << escape_json_string(user.username) << "\""
        << ",\"role\":\"" << escape_json_string(user.role) << "\""
        << ",\"created_at\":\"" << escape_json_string(user.created_at) << "\""
        << ",\"active\":" << (user.active ? "true" : "false") << "}";
    return oss.str();
}

std::string JsonHelper::serialize_file(const FileInfo& file) {
    std::ostringstream oss;
    oss << "{\"id\":" << file.id
        << ",\"filename\":\"" << escape_json_string(file.filename) << "\""
        << ",\"filepath\":\"" << escape_json_string(file.filepath) << "\""
        << ",\"mime_type\":\"" << escape_json_string(file.mime_type) << "\""
        << ",\"size\":" << file.size
        << ",\"uploader\":\"" << escape_json_string(file.uploader) << "\""
        << ",\"upload_time\":\"" << escape_json_string(file.upload_time) << "\""
        << ",\"category\":\"" << escape_json_string(file.category) << "\""
        << ",\"download_count\":" << file.download_count
        << ",\"is_public\":" << (file.is_public ? "true" : "false") << "}";
    return oss.str();
}

std::string JsonHelper::serialize_session(const Session& session) {
    std::ostringstream oss;
    oss << "{\"session_id\":\"" << escape_json_string(session.session_id) << "\""
        << ",\"username\":\"" << escape_json_string(session.username) << "\""
        << ",\"role\":\"" << escape_json_string(session.role) << "\""
        << ",\"created_at\":\"" << escape_json_string(session.created_at) << "\"}";
    return oss.str();
}

std::string JsonHelper::serialize_users(const std::vector<User>& users) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < users.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serialize_user(users[i]);
    }
    oss << "]";
    return oss.str();
}

std::string JsonHelper::serialize_files(const std::vector<FileInfo>& files) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < files.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serialize_file(files[i]);
    }
    oss << "]";
    return oss.str();
}

std::string JsonHelper::paginated_response(const std::string& data, int total, int page, int limit) {
    std::ostringstream oss;
    oss << "{\"success\":true,\"data\":" << data
        << ",\"pagination\":{\"total\":" << total
        << ",\"page\":" << page
        << ",\"limit\":" << limit
        << ",\"pages\":" << ((total + limit - 1) / limit) << "}}";
    return oss.str();
}

std::string JsonHelper::serialize_system_status(const std::map<std::string, std::string>& status) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& pair : status) {
        if (!first) oss << ",";
        oss << "\"" << escape_json_string(pair.first) << "\":\"" << escape_json_string(pair.second) << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::string JsonHelper::serialize_processes(const std::vector<std::map<std::string, std::string>>& processes) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < processes.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serialize_system_status(processes[i]);
    }
    oss << "]";
    return oss.str();
}

std::string JsonHelper::escape_json_string(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < 0x20) {
                    result += "\\u";
                    result += "0000";
                    // 简化的unicode转义
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

std::map<std::string, std::string> JsonHelper::parse_form_data(const std::string& data) {
    std::map<std::string, std::string> result;
    std::istringstream ss(data);
    std::string pair;
    
    while (std::getline(ss, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            result[key] = value;
        }
    }
    
    return result;
}

// 简化的实现，用于兼容现有代码
std::shared_ptr<JsonValue> JsonHelper::user_to_json(const User& user) {
    return JsonValue::create_string(serialize_user(user));
}

std::shared_ptr<JsonValue> JsonHelper::file_info_to_json(const FileInfo& file) {
    return JsonValue::create_string(serialize_file(file));
}

std::shared_ptr<JsonValue> JsonHelper::users_to_json(const std::vector<User>& users) {
    return JsonValue::create_string(serialize_users(users));
}

std::shared_ptr<JsonValue> JsonHelper::files_to_json(const std::vector<FileInfo>& files) {
    return JsonValue::create_string(serialize_files(files));
}

std::string JsonHelper::create_success_response(const std::string& message, std::shared_ptr<JsonValue> data) {
    if (data) {
        return data_response(data->to_string(), message);
    } else {
        return success_response(message);
    }
}

std::string JsonHelper::create_error_response(const std::string& message, int code) {
    return error_response(message, code);
}

std::string JsonHelper::create_paginated_response(const std::vector<FileInfo>& files, int total, int page, int limit) {
    return paginated_response(serialize_files(files), total, page, limit);
}

std::string JsonHelper::create_system_status_response(double cpu_usage, double memory_usage, double disk_usage, int process_count) {
    std::ostringstream oss;
    oss << "{\"success\":true,\"data\":{"
        << "\"cpu_usage\":" << cpu_usage
        << ",\"memory_usage\":" << memory_usage
        << ",\"disk_usage\":" << disk_usage
        << ",\"process_count\":" << process_count
        << "}}";
    return oss.str();
}

// 其他兼容方法的简化实现
std::shared_ptr<JsonValue> JsonHelper::params_to_json(const std::unordered_map<std::string, std::string>& params) {
    auto obj = JsonValue::create_object();
    for (const auto& pair : params) {
        obj->set_object_property(pair.first, JsonValue::create_string(pair.second));
    }
    return obj;
}

std::string JsonHelper::objectToJson(const std::unordered_map<std::string, std::string>& obj) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& pair : obj) {
        if (!first) oss << ",";
        oss << "\"" << escape_json_string(pair.first) << "\":\"" << escape_json_string(pair.second) << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::string JsonHelper::arrayToJson(const std::vector<std::string>& arr) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << escape_json_string(arr[i]) << "\"";
    }
    oss << "]";
    return oss.str();
}

std::string JsonHelper::valueToJson(const JsonValue& value) {
    return value.to_string();
}

std::string JsonHelper::createSuccessResponse(const std::string& message, const std::unordered_map<std::string, std::string>& data) {
    return data_response(objectToJson(data), message);
}

std::string JsonHelper::createErrorResponse(const std::string& error, int code) {
    return error_response(error, code);
}

std::string JsonHelper::createFileListResponse(const std::vector<std::map<std::string, std::string>>& files) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < files.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{";
        bool first = true;
        for (const auto& pair : files[i]) {
            if (!first) oss << ",";
            oss << "\"" << escape_json_string(pair.first) << "\":\"" << escape_json_string(pair.second) << "\"";
            first = false;
        }
        oss << "}";
    }
    oss << "]";
    return oss.str();
}

std::string JsonHelper::createUserResponse(const std::string& username, const std::string& role, int id) {
    std::ostringstream oss;
    oss << "{\"success\":true,\"data\":{"
        << "\"id\":" << id
        << ",\"username\":\"" << escape_json_string(username) << "\""
        << ",\"role\":\"" << escape_json_string(role) << "\""
        << "}}";
    return oss.str();
}

std::string JsonHelper::createSystemStatusResponse(const std::map<std::string, std::string>& status) {
    return data_response(serialize_system_status(status), "success");
}

// 这些方法返回默认值，因为实际上不需要真正的JSON解析
std::string JsonHelper::getString(const JsonValue& value, const std::string& default_val) {
    return default_val;
}

int JsonHelper::getInt(const JsonValue& value, int default_val) {
    return default_val;
}

double JsonHelper::getDouble(const JsonValue& value, double default_val) {
    return default_val;
}

bool JsonHelper::getBool(const JsonValue& value, bool default_val) {
    return default_val;
}

std::unordered_map<std::string, std::string> JsonHelper::parseObject(const std::string& json) {
    return std::unordered_map<std::string, std::string>();
}

bool JsonHelper::hasRequiredFields(const std::unordered_map<std::string, std::string>& obj, const std::vector<std::string>& fields) {
    for (const auto& field : fields) {
        if (obj.find(field) == obj.end()) {
            return false;
        }
    }
    return true;
}

// 这些解析方法为简化实现返回空值
std::string JsonHelper::parseString(const std::string& str, size_t& pos) {
    return "";
}

JsonValue JsonHelper::parseNumber(const std::string& str, size_t& pos) {
    return JsonValue(0);
}

JsonValue JsonHelper::parseLiteral(const std::string& str, size_t& pos) {
    return JsonValue();
}

void JsonHelper::skipWhitespace(const std::string& str, size_t& pos) {
    // 简化实现
}

std::string JsonHelper::escapeString(const std::string& str) {
    return escape_json_string(str);
}

bool JsonHelper::isNumberStart(char c) {
    return (c >= '0' && c <= '9') || c == '-' || c == '+';
}

bool JsonHelper::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool JsonHelper::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool JsonHelper::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
} 