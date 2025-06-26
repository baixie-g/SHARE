#include "json_helper.h"
#include <sstream>
#include <algorithm>

std::string JsonHelper::objectToJson(const JsonObject& obj) {
    std::stringstream ss;
    ss << "{";
    
    bool first = true;
    for (const auto& pair : obj) {
        if (!first) ss << ",";
        ss << "\"" << escapeString(pair.first) << "\":" << valueToJson(pair.second);
        first = false;
    }
    
    ss << "}";
    return ss.str();
}

std::string JsonHelper::arrayToJson(const JsonArray& arr) {
    std::stringstream ss;
    ss << "[";
    
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) ss << ",";
        ss << valueToJson(arr[i]);
    }
    
    ss << "]";
    return ss.str();
}

std::string JsonHelper::valueToJson(const JsonValue& value) {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return "\"" + escapeString(v) + "\"";
        } else if constexpr (std::is_same_v<T, int>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, bool>) {
            return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
            return "null";
        }
        return "null";
    }, value);
}

std::string JsonHelper::createSuccessResponse(const std::string& message, const JsonObject& data) {
    JsonObject response;
    response["success"] = true;
    response["message"] = message;
    
    if (!data.empty()) {
        response["data"] = objectToJson(data);
    }
    
    return objectToJson(response);
}

std::string JsonHelper::createErrorResponse(const std::string& error, int code) {
    JsonObject response;
    response["success"] = false;
    response["error"] = error;
    response["code"] = code;
    
    return objectToJson(response);
}

std::string JsonHelper::createFileListResponse(const std::vector<std::map<std::string, std::string>>& files) {
    std::stringstream ss;
    ss << "{\"success\":true,\"data\":[";
    
    for (size_t i = 0; i < files.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "{";
        
        bool first = true;
        for (const auto& pair : files[i]) {
            if (!first) ss << ",";
            ss << "\"" << escapeString(pair.first) << "\":\"" << escapeString(pair.second) << "\"";
            first = false;
        }
        
        ss << "}";
    }
    
    ss << "]}";
    return ss.str();
}

std::string JsonHelper::createUserResponse(const std::string& username, const std::string& role, int id) {
    JsonObject user;
    user["id"] = id;
    user["username"] = username;
    user["role"] = role;
    
    JsonObject response;
    response["success"] = true;
    response["data"] = objectToJson(user);
    
    return objectToJson(response);
}

std::string JsonHelper::createSystemStatusResponse(const std::map<std::string, std::string>& status) {
    std::stringstream ss;
    ss << "{\"success\":true,\"data\":{";
    
    bool first = true;
    for (const auto& pair : status) {
        if (!first) ss << ",";
        ss << "\"" << escapeString(pair.first) << "\":\"" << escapeString(pair.second) << "\"";
        first = false;
    }
    
    ss << "}}";
    return ss.str();
}

std::string JsonHelper::getString(const JsonValue& value, const std::string& default_val) {
    if (const auto* str = std::get_if<std::string>(&value)) {
        return *str;
    }
    return default_val;
}

int JsonHelper::getInt(const JsonValue& value, int default_val) {
    if (const auto* i = std::get_if<int>(&value)) {
        return *i;
    }
    return default_val;
}

double JsonHelper::getDouble(const JsonValue& value, double default_val) {
    if (const auto* d = std::get_if<double>(&value)) {
        return *d;
    }
    return default_val;
}

bool JsonHelper::getBool(const JsonValue& value, bool default_val) {
    if (const auto* b = std::get_if<bool>(&value)) {
        return *b;
    }
    return default_val;
}

JsonHelper::JsonObject JsonHelper::parseObject(const std::string& json) {
    JsonObject obj;
    
    // 简单的JSON解析实现
    size_t pos = json.find('{');
    if (pos == std::string::npos) return obj;
    
    pos++;
    skipWhitespace(json, pos);
    
    while (pos < json.length() && json[pos] != '}') {
        skipWhitespace(json, pos);
        if (pos >= json.length()) break;
        
        // 解析键
        if (json[pos] != '"') break;
        std::string key = parseString(json, pos);
        
        skipWhitespace(json, pos);
        if (pos >= json.length() || json[pos] != ':') break;
        pos++;
        
        skipWhitespace(json, pos);
        
        // 解析值
        JsonValue value;
        if (json[pos] == '"') {
            value = parseString(json, pos);
        } else if (isNumberStart(json[pos])) {
            value = parseNumber(json, pos);
        } else if (isAlpha(json[pos])) {
            value = parseLiteral(json, pos);
        }
        
        obj[key] = value;
        
        skipWhitespace(json, pos);
        if (pos < json.length() && json[pos] == ',') {
            pos++;
        }
    }
    
    return obj;
}

bool JsonHelper::hasRequiredFields(const JsonObject& obj, const std::vector<std::string>& fields) {
    for (const auto& field : fields) {
        if (obj.find(field) == obj.end()) {
            return false;
        }
    }
    return true;
}

std::string JsonHelper::parseString(const std::string& str, size_t& pos) {
    if (pos >= str.length() || str[pos] != '"') {
        return "";
    }
    
    pos++; // 跳过开始的引号
    std::string result;
    
    while (pos < str.length() && str[pos] != '"') {
        if (str[pos] == '\\' && pos + 1 < str.length()) {
            pos++;
            switch (str[pos]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                default: result += str[pos]; break;
            }
        } else {
            result += str[pos];
        }
        pos++;
    }
    
    if (pos < str.length()) pos++; // 跳过结束的引号
    return result;
}

JsonValue JsonHelper::parseNumber(const std::string& str, size_t& pos) {
    size_t start = pos;
    bool isFloat = false;
    
    if (str[pos] == '-') pos++;
    
    while (pos < str.length() && isDigit(str[pos])) {
        pos++;
    }
    
    if (pos < str.length() && str[pos] == '.') {
        isFloat = true;
        pos++;
        while (pos < str.length() && isDigit(str[pos])) {
            pos++;
        }
    }
    
    std::string numStr = str.substr(start, pos - start);
    
    if (isFloat) {
        return std::stod(numStr);
    } else {
        return std::stoi(numStr);
    }
}

JsonValue JsonHelper::parseLiteral(const std::string& str, size_t& pos) {
    size_t start = pos;
    
    while (pos < str.length() && isAlpha(str[pos])) {
        pos++;
    }
    
    std::string literal = str.substr(start, pos - start);
    
    if (literal == "true") return true;
    if (literal == "false") return false;
    if (literal == "null") return nullptr;
    
    return std::string(literal);
}

void JsonHelper::skipWhitespace(const std::string& str, size_t& pos) {
    while (pos < str.length() && isWhitespace(str[pos])) {
        pos++;
    }
}

std::string JsonHelper::escapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            default: result += c; break;
        }
    }
    return result;
}

bool JsonHelper::isNumberStart(char c) {
    return isDigit(c) || c == '-';
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