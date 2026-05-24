#pragma once
#include <string>
#include <functional>
#include <oatpp/core/Types.hpp>

class HashUtils {
public:
    // 计算字符串内容的哈希值
    static std::string hashContent(const oatpp::String& content) {
        if (!content || content->empty()) {
            return "";
        }
        std::hash<std::string> hasher;
        return std::to_string(hasher(content->c_str()));
    }

    // 计算 C 风格字符串的哈希值
    static std::string hashContent(const char* content) {
        if (!content || content[0] == '\0') {
            return "";
        }
        std::hash<std::string> hasher;
        return std::to_string(hasher(content));
    }

    // 计算标准字符串的哈希值
    static std::string hashContent(const std::string& content) {
        if (content.empty()) {
            return "";
        }
        std::hash<std::string> hasher;
        return std::to_string(hasher(content));
    }
};