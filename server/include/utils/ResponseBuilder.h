#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace snake {

/**
 * @brief 统一响应构造器
 */
class ResponseBuilder {
public:
    // 成功响应
    static nlohmann::json success(const nlohmann::json& data = nullptr);
    
    // 错误响应
    static nlohmann::json error(int code, const std::string& msg, 
                                const nlohmann::json& data = nullptr);
    
    // 常见错误响应
    static nlohmann::json badRequest(const std::string& msg);
    static nlohmann::json unauthorized(const std::string& msg = "unauthorized");
    static nlohmann::json forbidden(const std::string& msg = "forbidden");
    static nlohmann::json notFound(const std::string& msg = "not found");
    static nlohmann::json conflict(const std::string& msg);
    static nlohmann::json tooManyRequests(const std::string& msg, int retryAfter);
    static nlohmann::json internalError(const std::string& msg = "internal server error");
    static nlohmann::json serviceUnavailable(const std::string& msg = "service unavailable");

private:
    static nlohmann::json buildResponse(int code, const std::string& msg, 
                                        const nlohmann::json& data);
};

} // namespace snake
