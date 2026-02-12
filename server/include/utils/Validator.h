#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace snake {

/**
 * @brief 输入验证器
 */
class Validator {
public:
    // 洛谷剪贴板验证
    static bool validateLuoguPaste(const std::string& uid, const std::string& paste);
    
    // 参数验证
    static bool isValidUid(const std::string& uid);
    static bool isValidPlayerName(const std::string& name);
    static bool isValidColor(const std::string& color);
    static bool isValidDirection(const std::string& direction);
    
    // JSON 字段验证
    static bool hasRequiredFields(const nlohmann::json& j, 
                                  const std::vector<std::string>& fields);

private:
    static bool isHexColor(const std::string& color);
    static std::string fetchLuoguPaste(const std::string& paste);
    static std::string parseHttpResponse(const std::string& response);
    static std::string extractTextFromHtml(const std::string& html);
    static nlohmann::json parseHtmlForPasteData(const std::string& html, const std::string& pasteId);
    static std::string urlDecode(const std::string& str);
};

} // namespace snake
