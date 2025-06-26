#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>

// 简单的JSON构建器
class JsonBuilder {
public:
    JsonBuilder();
    
    void add(const std::string& key, const std::string& value);
    void add(const std::string& key, int value);
    void add(const std::string& key, long value);
    void add(const std::string& key, double value);
    void add(const std::string& key, bool value);
    void add_array(const std::string& key);
    void add_to_array(const std::string& value);
    void add_to_array(int value);
    void add_object(const std::string& key);
    void end_object();
    
    std::string build();
    
private:
    std::ostringstream json_;
    bool first_item_;
    std::vector<bool> in_array_;
    std::vector<bool> array_first_;
    
    void add_comma();
    std::string escape_string(const std::string& str);
};

// 简单的JSON解析器
class JsonParser {
public:
    explicit JsonParser(const std::string& json);
    
    bool has_key(const std::string& key);
    std::string get_string(const std::string& key, const std::string& default_value = "");
    int get_int(const std::string& key, int default_value = 0);
    double get_double(const std::string& key, double default_value = 0.0);
    bool get_bool(const std::string& key, bool default_value = false);
    
private:
    std::unordered_map<std::string, std::string> data_;
    
    void parse(const std::string& json);
    std::string trim(const std::string& str);
    std::string unescape_string(const std::string& str);
}; 