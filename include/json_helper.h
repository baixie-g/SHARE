#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include <string>
#include <map>
#include <vector>
#include <variant>

// JSON值类型
using JsonValue = std::variant<std::string, int, double, bool, std::nullptr_t>;

/**
 * 简单的JSON处理类
 * 提供基础的JSON序列化和反序列化功能
 */
class JsonHelper {
public:
    // JSON对象类型
    using JsonObject = std::map<std::string, JsonValue>;
    using JsonArray = std::vector<JsonValue>;

    // === 序列化功能 ===
    // 将对象转换为JSON字符串
    static std::string objectToJson(const JsonObject& obj);
    
    // 将数组转换为JSON字符串
    static std::string arrayToJson(const JsonArray& arr);
    
    // 将值转换为JSON字符串
    static std::string valueToJson(const JsonValue& value);

    // === 反序列化功能 ===
    // 从JSON字符串解析对象
    static JsonObject parseObject(const std::string& json);
    
    // 从JSON字符串解析数组
    static JsonArray parseArray(const std::string& json);
    
    // 从JSON字符串解析值
    static JsonValue parseValue(const std::string& json);

    // === 便利函数 ===
    // 创建成功响应
    static std::string createSuccessResponse(const std::string& message = "Success", 
                                           const JsonObject& data = {});
    
    // 创建错误响应
    static std::string createErrorResponse(const std::string& error, int code = 400);
    
    // 创建文件列表响应
    static std::string createFileListResponse(const std::vector<std::map<std::string, std::string>>& files);
    
    // 创建用户信息响应
    static std::string createUserResponse(const std::string& username, const std::string& role, int id);
    
    // 创建系统状态响应
    static std::string createSystemStatusResponse(const std::map<std::string, std::string>& status);

    // === 类型转换辅助 ===
    // 获取字符串值
    static std::string getString(const JsonValue& value, const std::string& default_val = "");
    
    // 获取整数值
    static int getInt(const JsonValue& value, int default_val = 0);
    
    // 获取浮点数值
    static double getDouble(const JsonValue& value, double default_val = 0.0);
    
    // 获取布尔值
    static bool getBool(const JsonValue& value, bool default_val = false);

    // === 验证功能 ===
    // 检查JSON格式是否有效
    static bool isValidJson(const std::string& json);
    
    // 检查对象是否包含必需字段
    static bool hasRequiredFields(const JsonObject& obj, const std::vector<std::string>& fields);

private:
    // 解析字符串，处理转义字符
    static std::string parseString(const std::string& str, size_t& pos);
    
    // 解析数字
    static JsonValue parseNumber(const std::string& str, size_t& pos);
    
    // 解析布尔值或null
    static JsonValue parseLiteral(const std::string& str, size_t& pos);
    
    // 跳过空白字符
    static void skipWhitespace(const std::string& str, size_t& pos);
    
    // 转义字符串
    static std::string escapeString(const std::string& str);
    
    // 反转义字符串
    static std::string unescapeString(const std::string& str);
    
    // 检查字符是否为数字开始字符
    static bool isNumberStart(char c);
    
    // 检查字符是否为字母
    static bool isAlpha(char c);
    
    // 检查字符是否为数字
    static bool isDigit(char c);
    
    // 检查字符是否为空白字符
    static bool isWhitespace(char c);
};

#endif // JSON_HELPER_H 