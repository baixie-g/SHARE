#include "json_helper.h"
#include <iostream>
#include <regex>

// JsonBuilder 实现
JsonBuilder::JsonBuilder() : first_item_(true) {
    json_ << "{";
}

void JsonBuilder::add(const std::string& key, const std::string& value) {
    add_comma();
    json_ << "\"" << escape_string(key) << "\":\"" << escape_string(value) << "\"";
}

void JsonBuilder::add(const std::string& key, int value) {
    add_comma();
    json_ << "\"" << escape_string(key) << "\":" << value;
}

void JsonBuilder::add(const std::string& key, long value) {
    add_comma();
    json_ << "\"" << escape_string(key) << "\":" << value;
}

void JsonBuilder::add(const std::string& key, double value) {
    add_comma();
    json_ << "\"" << escape_string(key) << "\":" << value;
}

void JsonBuilder::add(const std::string& key, bool value) {
    add_comma();
    json_ << "\"" << escape_string(key) << "\":" << (value ? "true" : "false");
}

void JsonBuilder::add_array(const std::string& key) {
    add_comma();
    json_ << "\"" << escape_string(key) << "\":[";
    in_array_.push_back(true);
    array_first_.push_back(true);
}

void JsonBuilder::add_to_array(const std::string& value) {
    if (!in_array_.empty() && in_array_.back()) {
        if (!array_first_.back()) {
            json_ << ",";
        }
        json_ << "\"" << escape_string(value) << "\"";
        array_first_.back() = false;
    }
}

void JsonBuilder::add_to_array(int value) {
    if (!in_array_.empty() && in_array_.back()) {
        if (!array_first_.back()) {
            json_ << ",";
        }
        json_ << value;
        array_first_.back() = false;
    }
}

void JsonBuilder::add_object(const std::string& key) {
    add_comma();
    json_ << "\"" << escape_string(key) << "\":{";
    first_item_ = true;
}

void JsonBuilder::end_object() {
    json_ << "}";
    first_item_ = false;
}

std::string JsonBuilder::build() {
    // 关闭所有打开的数组
    while (!in_array_.empty()) {
        json_ << "]";
        in_array_.pop_back();
        array_first_.pop_back();
    }
    
    json_ << "}";
    return json_.str();
}

void JsonBuilder::add_comma() {
    if (!first_item_) {
        json_ << ",";
    }
    first_item_ = false;
}

std::string JsonBuilder::escape_string(const std::string& str) {
    std::string escaped = str;
    
    // 替换特殊字符
    size_t pos = 0;
    while ((pos = escaped.find("\\", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\\");
        pos += 2;
    }
    
    pos = 0;
    while ((pos = escaped.find("\"", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    pos = 0;
    while ((pos = escaped.find("\n", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\n");
        pos += 2;
    }
    
    pos = 0;
    while ((pos = escaped.find("\r", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\r");
        pos += 2;
    }
    
    pos = 0;
    while ((pos = escaped.find("\t", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\t");
        pos += 2;
    }
    
    return escaped;
}

// JsonParser 实现
JsonParser::JsonParser(const std::string& json) {
    parse(json);
}

bool JsonParser::has_key(const std::string& key) {
    return data_.find(key) != data_.end();
}

std::string JsonParser::get_string(const std::string& key, const std::string& default_value) {
    auto it = data_.find(key);
    if (it != data_.end()) {
        return unescape_string(it->second);
    }
    return default_value;
}

int JsonParser::get_int(const std::string& key, int default_value) {
    auto it = data_.find(key);
    if (it != data_.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            return default_value;
        }
    }
    return default_value;
}

double JsonParser::get_double(const std::string& key, double default_value) {
    auto it = data_.find(key);
    if (it != data_.end()) {
        try {
            return std::stod(it->second);
        } catch (const std::exception&) {
            return default_value;
        }
    }
    return default_value;
}

bool JsonParser::get_bool(const std::string& key, bool default_value) {
    auto it = data_.find(key);
    if (it != data_.end()) {
        std::string value = trim(it->second);
        return value == "true";
    }
    return default_value;
}

void JsonParser::parse(const std::string& json) {
    // 简单的键值对解析，使用普通字符串而不是原始字符串字面量来避免问题
    std::regex pair_regex("\"([^\"]+)\"\\s*:\\s*\"([^\"]*)\"");
    std::regex number_regex("\"([^\"]+)\"\\s*:\\s*([0-9.-]+)");
    std::regex bool_regex("\"([^\"]+)\"\\s*:\\s*(true|false)");
    
    std::smatch match;
    std::string::const_iterator search_start = json.cbegin();
    
    // 匹配字符串值
    while (std::regex_search(search_start, json.cend(), match, pair_regex)) {
        data_[match[1].str()] = match[2].str();
        search_start = match.suffix().first;
    }
    
    // 重置搜索位置
    search_start = json.cbegin();
    
    // 匹配数字值
    while (std::regex_search(search_start, json.cend(), match, number_regex)) {
        if (data_.find(match[1].str()) == data_.end()) {
            data_[match[1].str()] = match[2].str();
        }
        search_start = match.suffix().first;
    }
    
    // 重置搜索位置
    search_start = json.cbegin();
    
    // 匹配布尔值
    while (std::regex_search(search_start, json.cend(), match, bool_regex)) {
        if (data_.find(match[1].str()) == data_.end()) {
            data_[match[1].str()] = match[2].str();
        }
        search_start = match.suffix().first;
    }
}

std::string JsonParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::string JsonParser::unescape_string(const std::string& str) {
    std::string unescaped = str;
    
    size_t pos = 0;
    while ((pos = unescaped.find("\\\"", pos)) != std::string::npos) {
        unescaped.replace(pos, 2, "\"");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = unescaped.find("\\\\", pos)) != std::string::npos) {
        unescaped.replace(pos, 2, "\\");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = unescaped.find("\\n", pos)) != std::string::npos) {
        unescaped.replace(pos, 2, "\n");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = unescaped.find("\\r", pos)) != std::string::npos) {
        unescaped.replace(pos, 2, "\r");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = unescaped.find("\\t", pos)) != std::string::npos) {
        unescaped.replace(pos, 2, "\t");
        pos += 1;
    }
    
    return unescaped;
} 