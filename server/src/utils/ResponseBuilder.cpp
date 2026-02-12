#include "../include/utils/ResponseBuilder.h"

namespace snake {

nlohmann::json ResponseBuilder::success(const nlohmann::json& data) {
    return buildResponse(0, "success", data);
}

nlohmann::json ResponseBuilder::error(int code, const std::string& msg, 
                                      const nlohmann::json& data) {
    return buildResponse(code, msg, data);
}

nlohmann::json ResponseBuilder::badRequest(const std::string& msg) {
    return buildResponse(400, msg.empty() ? "bad request" : msg, nullptr);
}

nlohmann::json ResponseBuilder::unauthorized(const std::string& msg) {
    return buildResponse(401, msg, nullptr);
}

nlohmann::json ResponseBuilder::forbidden(const std::string& msg) {
    return buildResponse(403, msg, nullptr);
}

nlohmann::json ResponseBuilder::notFound(const std::string& msg) {
    return buildResponse(404, msg, nullptr);
}

nlohmann::json ResponseBuilder::conflict(const std::string& msg) {
    return buildResponse(409, msg, nullptr);
}

nlohmann::json ResponseBuilder::tooManyRequests(const std::string& msg, int retryAfter) {
    nlohmann::json data = {{"retry_after", retryAfter}};
    return buildResponse(429, msg, data);
}

nlohmann::json ResponseBuilder::internalError(const std::string& msg) {
    return buildResponse(500, msg, nullptr);
}

nlohmann::json ResponseBuilder::serviceUnavailable(const std::string& msg) {
    return buildResponse(503, msg, nullptr);
}

nlohmann::json ResponseBuilder::buildResponse(int code, const std::string& msg,
                                              const nlohmann::json& data) {
    nlohmann::json response;
    response["code"] = code;
    response["msg"] = msg;
    response["data"] = data.is_null() ? nullptr : data;
    return response;
}

} // namespace snake
