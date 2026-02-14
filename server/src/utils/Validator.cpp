#include "utils/Validator.h"
#include "models/Config.h"
#include "utils/Logger.h"
#include <regex>
#include <algorithm>
#include <sstream>

// 使用 Crow 的 SimpleApp 进行 HTTP 客户端请求
#include <asio.hpp>
#include <asio/ssl.hpp>

namespace snake {

/**
 * @brief 验证洛谷剪贴板
 * @param uid 洛谷用户 ID
 * @param paste 剪贴板后缀
 * @return 验证成功返回 true，否则返回 false
 */
bool Validator::validateLuoguPaste(const std::string& uid, const std::string& paste) {
    try {
        // 0. 万能 paste：命中后直接通过（不校验 uid）
        const std::string& universalPaste = Config::getInstance().getAuth().universalPaste;
        if (!universalPaste.empty() && paste == universalPaste) {
            LOG_INFO("Universal paste accepted for UID: " + uid);
            return true;
        }

        // 1. 基础参数验证
        if (!isValidUid(uid)) {
            LOG_WARNING("Invalid UID format: " + uid);
            return false;
        }

        if (paste.empty() || paste.length() > 50) {
            LOG_WARNING("Invalid paste format");
            return false;
        }

        // 2. 构造剪贴板 URL
        std::string url = "https://www.luogu.com/paste/" + paste;
        LOG_INFO("Validating Luogu paste: " + url);

        // 3. 发起 HTTPS 请求获取HTML页面
        std::string htmlContent = fetchLuoguPaste(paste);
        
        if (htmlContent.empty()) {
            LOG_WARNING("Failed to fetch paste content or paste not found");
            return false;
        }

        // 4. 解析 HTML 中的 JSON 数据
        nlohmann::json pasteData = parseHtmlForPasteData(htmlContent, paste);
        
        if (pasteData.is_null()) {
            LOG_WARNING("Failed to parse paste data from HTML");
            return false;
        }

        // 5. 验证发布者 UID
        if (!pasteData.contains("user") || !pasteData["user"].contains("uid")) {
            LOG_WARNING("Paste data does not contain user UID");
            return false;
        }

        int authorUid = pasteData["user"]["uid"].get<int>();
        if (std::to_string(authorUid) != uid) {
            LOG_WARNING("Paste author UID (" + std::to_string(authorUid) + 
                       ") does not match provided UID (" + uid + ")");
            return false;
        }

        // 6. 验证剪贴板内容
        if (!pasteData.contains("data")) {
            LOG_WARNING("Paste data does not contain content");
            return false;
        }

        std::string pasteContent = pasteData["data"].get<std::string>();
        
        // 7. 检查剪贴板内容是否包含所需验证文本
        const std::string& expectedText = Config::getInstance().getAuth().luoguValidationText;
        
        if (pasteContent.find(expectedText) == std::string::npos) {
            LOG_WARNING("Paste content does not contain expected validation text");
            return false;
        }

        LOG_INFO("Luogu paste validation successful for UID: " + uid);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Exception during paste validation: " + std::string(e.what()));
        return false;
    }
}

/**
 * @brief 获取洛谷剪贴板内容
 * @param paste 剪贴板后缀
 * @return 剪贴板内容，失败返回空字符串
 */
std::string Validator::fetchLuoguPaste(const std::string& paste) {
    try {
        // 使用 ASIO 进行 HTTPS 请求
        asio::io_context io_context;
        asio::ssl::context ssl_context(asio::ssl::context::tlsv12_client);
        
        // 加载默认证书
        ssl_context.set_default_verify_paths();
        ssl_context.set_verify_mode(asio::ssl::verify_none); // 简化处理，生产环境应验证证书

        asio::ip::tcp::resolver resolver(io_context);
        asio::ssl::stream<asio::ip::tcp::socket> socket(io_context, ssl_context);

        // 解析域名
        auto endpoints = resolver.resolve("www.luogu.com", "443");
        
        // 设置 SNI（Server Name Indication）
        SSL_set_tlsext_host_name(socket.native_handle(), "www.luogu.com");
        
        // 连接到服务器
        asio::connect(socket.lowest_layer(), endpoints);
        
        // SSL 握手
        socket.handshake(asio::ssl::stream_base::client);

        // 构造 HTTP GET 请求（模拟浏览器）
        std::string request = 
            "GET /paste/" + paste + " HTTP/1.1\r\n"
            "Host: www.luogu.com\r\n"
            "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\r\n"
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
            "Accept-Language: zh-CN,zh;q=0.9,en;q=0.8\r\n"
            "Connection: close\r\n"
            "\r\n";

        // 发送请求
        asio::write(socket, asio::buffer(request));

        // 读取响应
        std::string response;
        asio::error_code error;
        char buf[1024];
        
        while (true) {
            size_t len = socket.read_some(asio::buffer(buf), error);
            if (error == asio::error::eof) {
                response.append(buf, len);
                break;
            } else if (error) {
                throw asio::system_error(error);
            }
            response.append(buf, len);
        }

        // 解析 HTTP 响应
        return parseHttpResponse(response);

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to fetch Luogu paste: " + std::string(e.what()));
        return "";
    }
}

/**
 * @brief 解析 HTTP 响应，提取正文内容
 * @param response 原始 HTTP 响应
 * @return 响应正文内容
 */
std::string Validator::parseHttpResponse(const std::string& response) {
    // 查找响应头和正文的分隔符
    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return "";
    }

    // 检查状态码
    if (response.find("HTTP/1.1 200") == std::string::npos &&
        response.find("HTTP/1.0 200") == std::string::npos) {
        LOG_WARNING("HTTP request failed, non-200 status code");
        return "";
    }

    // 提取正文
    std::string body = response.substr(headerEnd + 4);
    
    // 返回原始 HTML 内容（不要提取纯文本，后续需要解析 JavaScript）
    return body;
}

/**
 * @brief 从 HTML 中解析洛谷剪贴板的 JSON 数据
 * @param html HTML 内容
 * @param pasteId 剪贴板 ID
 * @return 剪贴板的 JSON 数据，失败返回 null
 */
nlohmann::json Validator::parseHtmlForPasteData(const std::string& html, const std::string& pasteId) {
    try {
        // 查找 window._feInjection = JSON.parse(decodeURIComponent(...
        std::string searchPattern = "window._feInjection = JSON.parse(decodeURIComponent(\"";
        size_t jsonStart = html.find(searchPattern);
        
        if (jsonStart == std::string::npos) {
            LOG_WARNING("Could not find _feInjection in HTML");
            return nlohmann::json();
        }
        
        jsonStart += searchPattern.length();
        
        // 找到 JSON 字符串的结束位置（\")))
        size_t jsonEnd = html.find("\"));window._feConfigVersion", jsonStart);
        if (jsonEnd == std::string::npos) {
            jsonEnd = html.find("\"));window", jsonStart);
        }
        if (jsonEnd == std::string::npos) {
            jsonEnd = html.find("\"))", jsonStart);
        }
        
        if (jsonEnd == std::string::npos) {
            LOG_WARNING("Could not find end of _feInjection JSON");
            return nlohmann::json();
        }
        
        std::string encodedJson = html.substr(jsonStart, jsonEnd - jsonStart);
        
        // URL 解码
        std::string decodedJson = urlDecode(encodedJson);
        
        // 解析 JSON
        nlohmann::json rootData = nlohmann::json::parse(decodedJson);
        
        // 单个剪贴板页面：导航到 currentData.paste
        if (rootData.contains("currentData") && 
            rootData["currentData"].contains("paste")) {
            return rootData["currentData"]["paste"];
        }
        
        // 剪贴板列表页面：导航到 currentData.pastes.result 数组
        if (rootData.contains("currentData") && 
            rootData["currentData"].contains("pastes") &&
            rootData["currentData"]["pastes"].contains("result")) {
            auto& results = rootData["currentData"]["pastes"]["result"];
            
            // 查找匹配的剪贴板 ID
            for (const auto& item : results) {
                if (item.contains("id") && item["id"].get<std::string>() == pasteId) {
                    return item;
                }
            }
        }
        
        LOG_WARNING("JSON structure does not match expected format");
        return nlohmann::json();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse HTML for paste data: " + std::string(e.what()));
        return nlohmann::json();
    }
}

/**
 * @brief URL 解码
 * @param str URL 编码的字符串
 * @return 解码后的字符串
 */
std::string Validator::urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            // 解码 %XX
            std::string hex = str.substr(i + 1, 2);
            try {
                int value = std::stoi(hex, nullptr, 16);
                result += static_cast<char>(value);
                i += 2;
            } catch (...) {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    
    return result;
}

/**
 * @brief 从 HTML 中提取纯文本内容
 * @param html HTML 内容
 * @return 纯文本内容
 */
std::string Validator::extractTextFromHtml(const std::string& html) {
    // 简单的 HTML 标签去除（生产环境应使用专业的 HTML 解析器）
    std::string text = html;
    
    // 移除 <script> 和 <style> 标签及其内容
    std::regex scriptRegex("<script[^>]*>.*?</script>", std::regex::icase);
    text = std::regex_replace(text, scriptRegex, "");
    
    std::regex styleRegex("<style[^>]*>.*?</style>", std::regex::icase);
    text = std::regex_replace(text, styleRegex, "");
    
    // 移除所有 HTML 标签
    std::regex tagRegex("<[^>]*>");
    text = std::regex_replace(text, tagRegex, "");
    
    // 解码常见的 HTML 实体
    text = std::regex_replace(text, std::regex("&nbsp;"), " ");
    text = std::regex_replace(text, std::regex("&lt;"), "<");
    text = std::regex_replace(text, std::regex("&gt;"), ">");
    text = std::regex_replace(text, std::regex("&amp;"), "&");
    text = std::regex_replace(text, std::regex("&quot;"), "\"");
    
    return text;
}

/**
 * @brief 验证 UID 格式
 * @param uid 用户 ID
 * @return 格式正确返回 true
 */
bool Validator::isValidUid(const std::string& uid) {
    if (uid.empty() || uid.length() > 10) {
        return false;
    }
    
    // UID 应该是纯数字
    return std::all_of(uid.begin(), uid.end(), ::isdigit);
}

/**
 * @brief 验证玩家名称
 * @param name 玩家名称
 * @return 格式正确返回 true
 */
bool Validator::isValidPlayerName(const std::string& name) {
    // 长度检查：1-20 字符
    if (name.empty() || name.length() > 20) {
        return false;
    }
    
    // 不能包含控制字符
    for (char c : name) {
        if (std::iscntrl(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 验证颜色格式
 * @param color 颜色字符串（十六进制格式）
 * @return 格式正确返回 true
 */
bool Validator::isValidColor(const std::string& color) {
    return isHexColor(color);
}

/**
 * @brief 验证方向字符串
 * @param direction 方向字符串
 * @return 格式正确返回 true
 */
bool Validator::isValidDirection(const std::string& direction) {
    return direction == "up" || direction == "down" || 
           direction == "left" || direction == "right";
}

/**
 * @brief 检查 JSON 对象是否包含所需字段
 * @param j JSON 对象
 * @param fields 字段列表
 * @return 包含所有字段返回 true
 */
bool Validator::hasRequiredFields(const nlohmann::json& j, 
                                  const std::vector<std::string>& fields) {
    for (const auto& field : fields) {
        if (!j.contains(field)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 检查是否为有效的十六进制颜色
 * @param color 颜色字符串
 * @return 格式正确返回 true
 */
bool Validator::isHexColor(const std::string& color) {
    // 格式：#RRGGBB 或 #RGB
    std::regex hexColorRegex("^#([0-9A-Fa-f]{6}|[0-9A-Fa-f]{3})$");
    return std::regex_match(color, hexColorRegex);
}

} // namespace snake
