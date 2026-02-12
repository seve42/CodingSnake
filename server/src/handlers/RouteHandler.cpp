#include "../include/handlers/RouteHandler.h"
#include "../include/utils/ResponseBuilder.h"
#include "../include/utils/Validator.h"
#include "../include/utils/Logger.h"
#include "../include/utils/PerformanceMonitor.h"
#include "../include/models/Config.h"
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cctype>

namespace snake {

RouteHandler::RouteHandler(std::shared_ptr<GameManager> gameManager,
                           std::shared_ptr<PlayerManager> playerManager,
                           std::shared_ptr<MapManager> mapManager,
                           std::shared_ptr<LeaderboardManager> leaderboardManager)
    : gameManager_(gameManager)
    , playerManager_(playerManager)
    , mapManager_(mapManager)
    , leaderboardManager_(leaderboardManager) {
    LOG_INFO("RouteHandler initialized");
}

RouteHandler::~RouteHandler() {
    LOG_INFO("RouteHandler destroyed");
}

crow::response RouteHandler::handleStatus(const crow::request& req) {
    try {
        PerformanceMonitor::ScopedRequest metricsGuard("status");
        // 检查速率限制
        std::string clientIp = getClientIp(req);
        if (!checkRateLimit(clientIp, "status")) {
            LOG_WARNING("Rate limit exceeded for status endpoint from IP: " + clientIp);
            const auto& rateLimitConfig = Config::getInstance().getRateLimit();
            int retryAfter = rateLimiter_.getRetryAfter(
                "status:" + clientIp,
                rateLimitConfig.statusPerMinute,
                rateLimitConfig.statusWindowSeconds);
            return buildResponse(ResponseBuilder::tooManyRequests(
                "too many requests, please retry after " + std::to_string(retryAfter) + " seconds",
                retryAfter));
        }

        // 获取配置信息
        const auto& gameConfig = Config::getInstance().getGame();
        
        // 构建响应数据
        nlohmann::json data = {
            {"status", "running"},
            {"player_count", playerManager_->getPlayerCount()},
            {"map_size", {
                {"width", gameConfig.mapWidth},
                {"height", gameConfig.mapHeight}
            }},
            {"round_time", gameConfig.roundTimeMs}
        };

        LOG_DEBUG("Status requested from IP: " + clientIp);
        return buildResponse(ResponseBuilder::success(data));
    }
    catch (const std::exception& e) {
        return handleException(e);
    }
}

crow::response RouteHandler::handleLogin(const crow::request& req) {
    try {
        PerformanceMonitor::ScopedRequest metricsGuard("login");
        // 检查速率限制
        std::string clientIp = getClientIp(req);
        if (!checkRateLimit(clientIp, "login")) {
            LOG_WARNING("Rate limit exceeded for login endpoint from IP: " + clientIp);
            const auto& rateLimitConfig = Config::getInstance().getRateLimit();
            int retryAfter = rateLimiter_.getRetryAfter("login:" + clientIp, 
                           rateLimitConfig.loginPerHour, rateLimitConfig.loginWindowSeconds);
            return buildResponse(ResponseBuilder::tooManyRequests(
                "too many requests, please retry after " + std::to_string(retryAfter) + " seconds", 
                retryAfter));
        }

        // 解析请求参数
        nlohmann::json requestData;
        try {
            requestData = nlohmann::json::parse(req.body);
        } catch (const nlohmann::json::parse_error& e) {
            LOG_WARNING("Invalid JSON in login request: " + std::string(e.what()));
            return buildResponse(ResponseBuilder::badRequest("invalid json format"));
        }

        // 验证必需参数
        if (!requestData.contains("uid") || !requestData.contains("paste")) {
            LOG_WARNING("Missing required parameters in login request");
            return buildResponse(ResponseBuilder::badRequest("missing uid or paste parameter"));
        }

        std::string uid = requestData["uid"];
        std::string paste = requestData["paste"];

        // 参数基础验证
        if (uid.empty() || paste.empty()) {
            LOG_WARNING("Empty uid or paste in login request");
            return buildResponse(ResponseBuilder::badRequest("uid and paste cannot be empty"));
        }

        // 调用Validator验证洛谷身份
        if (!Validator::validateLuoguPaste(uid, paste)) {
            LOG_WARNING("Luogu validation failed for UID: " + uid);
            return buildResponse(ResponseBuilder::forbidden("authentication failed"));
        }

        // 调用PlayerManager登录
        std::string key = playerManager_->login(uid, paste);
        if (key.empty()) {
            LOG_ERROR("PlayerManager login failed for UID: " + uid);
            return buildResponse(ResponseBuilder::internalError("login failed"));
        }

        // 构造成功响应
        nlohmann::json data = {
            {"key", key}
        };

        LOG_INFO("Login successful for UID: " + uid + ", IP: " + clientIp);
        return buildResponse(ResponseBuilder::success(data));
    }
    catch (const std::exception& e) {
        return handleException(e);
    }
}

crow::response RouteHandler::handleJoin(const crow::request& req) {
    try {
        PerformanceMonitor::ScopedRequest metricsGuard("join");
        // 1. 解析请求参数
        nlohmann::json requestData;
        try {
            requestData = nlohmann::json::parse(req.body);
        } catch (const nlohmann::json::parse_error& e) {
            LOG_WARNING("Invalid JSON in join request: " + std::string(e.what()));
            return buildResponse(ResponseBuilder::badRequest("invalid json format"));
        }

        // 2. 验证必需参数
        if (!requestData.contains("key") || !requestData.contains("name")) {
            LOG_WARNING("Missing required parameters in join request");
            return buildResponse(ResponseBuilder::badRequest("missing key or name parameter"));
        }

        std::string key = requestData["key"];
        std::string name = requestData["name"];
        std::string color = requestData.value("color", ""); // 可选参数

        // 3. 参数基础验证
        if (key.empty()) {
            LOG_WARNING("Empty key in join request");
            return buildResponse(ResponseBuilder::badRequest("key cannot be empty"));
        }

        if (name.empty()) {
            LOG_WARNING("Empty name in join request");
            return buildResponse(ResponseBuilder::badRequest("name cannot be empty"));
        }

        // 4. 验证 key 有效性
        std::string uid;
        if (!playerManager_->validateKey(key, uid)) {
            LOG_WARNING("Invalid key in join request: " + key);
            return buildResponse(ResponseBuilder::unauthorized("invalid key"));
        }

        // 5. 速率限制检查（基于 key）
        if (!checkRateLimit(key, "join")) {
            LOG_WARNING("Rate limit exceeded for join endpoint, key: " + key);
            const auto& rateLimitConfig = Config::getInstance().getRateLimit();
            int retryAfter = rateLimiter_.getRetryAfter("join:" + key, 
                           rateLimitConfig.joinPerMinute, rateLimitConfig.joinWindowSeconds);
            return buildResponse(ResponseBuilder::tooManyRequests(
                "too many requests, please retry after " + std::to_string(retryAfter) + " seconds", 
                retryAfter));
        }

        // 6. 生成随机颜色（如未提供）
        // PlayerManager::join 方法会处理颜色生成和验证

        // 7. 调用 PlayerManager 加入游戏
        auto joinResult = playerManager_->join(key, name, color);
        
        if (!joinResult.success) {
            LOG_WARNING("Join failed for UID " + uid + ": " + joinResult.errorMsg);
            
            // 根据错误信息返回适当的HTTP状态码
            if (joinResult.errorMsg.find("already in game") != std::string::npos) {
                return buildResponse(ResponseBuilder::conflict(joinResult.errorMsg));
            } else if (joinResult.errorMsg.find("Invalid") != std::string::npos) {
                return buildResponse(ResponseBuilder::badRequest(joinResult.errorMsg));
            } else {
                return buildResponse(ResponseBuilder::internalError(joinResult.errorMsg));
            }
        }

        // 8. 获取玩家对象
        auto player = playerManager_->getPlayerById(joinResult.playerId);
        if (!player) {
            LOG_ERROR("Failed to get player after join: " + joinResult.playerId);
            return buildResponse(ResponseBuilder::internalError("failed to retrieve player data"));
        }

        // 9. 初始化蛇的位置
        const auto& gameConfig = Config::getInstance().getGame();
        const int safeRadius = 5; // 安全半径，确保周围没有其他蛇
        
        // 获取安全的初始位置
        Point spawnPos = mapManager_->getRandomSafePosition(
            playerManager_->getAllPlayers(), 
            safeRadius
        );
        
        // 初始化蛇并设置无敌回合数
        player->initSnake(spawnPos, gameConfig.initialSnakeLength);
        player->getSnake().setInvincibleRounds(gameConfig.invincibleRounds);
        
        // 设置随机初始方向
        std::vector<Direction> directions = {Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT};
        Direction initialDirection = directions[rand() % directions.size()];
        player->getSnake().setDirection(initialDirection);

        // 10. 将玩家添加到游戏管理器
        if (!gameManager_->addPlayer(player)) {
            LOG_ERROR("Failed to add player to game: " + joinResult.playerId);
            playerManager_->removePlayer(joinResult.playerId);
            return buildResponse(ResponseBuilder::internalError("failed to join game"));
        }

        // 11. 获取初始地图状态
        GameState currentState = gameManager_->getGameState();
        nlohmann::json mapStateJson = currentState.toJson();

        // 12. 构造成功响应
        nlohmann::json data = {
            {"token", joinResult.token},
            {"id", joinResult.playerId},
            {"initial_direction", DirectionUtils::toString(initialDirection)},
            {"map_state", mapStateJson}
        };

        LOG_INFO("Player successfully joined: UID=" + uid + ", Name=" + name + 
                 ", PlayerId=" + joinResult.playerId + ", Token=" + joinResult.token);
        
        return buildResponse(ResponseBuilder::success(data));
    }
    catch (const std::exception& e) {
        return handleException(e);
    }
}

crow::response RouteHandler::handleGetMap(const crow::request& req) {
    try {
        PerformanceMonitor::ScopedRequest metricsGuard("map");
        // 直接获取游戏状态，无需token验证
        GameState currentState = gameManager_->getGameState();
        nlohmann::json mapStateJson = currentState.toJson();
        
        // 5. 构造响应
        nlohmann::json data = {
            {"map_state", mapStateJson}
        };
        
        LOG_DEBUG("Map state requested (no token required)");
        return buildResponse(ResponseBuilder::success(data));
    }
    catch (const std::exception& e) {
        return handleException(e);
    }
}

crow::response RouteHandler::handleGetMapDelta(const crow::request& req) {
    try {
        PerformanceMonitor::ScopedRequest metricsGuard("map_delta");
        // 直接获取增量状态，无需token验证或任何参数
        nlohmann::json deltaStateJson = gameManager_->getDeltaState();
        
        // 构造响应
        nlohmann::json data = {
            {"delta_state", deltaStateJson}
        };
        
        LOG_DEBUG("Delta map state requested (no token required)");
        return buildResponse(ResponseBuilder::success(data));
    }
    catch (const std::exception& e) {
        return handleException(e);
    }
}

crow::response RouteHandler::handleMove(const crow::request& req) {
    try {
        PerformanceMonitor::ScopedRequest metricsGuard("move");
        // 1. 解析请求参数
        nlohmann::json requestData;
        try {
            requestData = nlohmann::json::parse(req.body);
        } catch (const nlohmann::json::parse_error& e) {
            LOG_WARNING("Invalid JSON in move request: " + std::string(e.what()));
            return buildResponse(ResponseBuilder::badRequest("invalid json format"));
        }

        // 2. 验证必需参数
        if (!requestData.contains("token") || !requestData.contains("direction")) {
            LOG_WARNING("Missing required parameters in move request");
            return buildResponse(ResponseBuilder::badRequest("missing token or direction parameter"));
        }

        std::string token = requestData["token"];
        std::string directionStr = requestData["direction"];

        // 3. 参数基础验证
        if (token.empty()) {
            LOG_WARNING("Empty token in move request");
            return buildResponse(ResponseBuilder::badRequest("token cannot be empty"));
        }

        if (directionStr.empty()) {
            LOG_WARNING("Empty direction in move request");
            return buildResponse(ResponseBuilder::badRequest("direction cannot be empty"));
        }

        // 4. 验证 token
        std::string playerId;
        if (!playerManager_->validateToken(token, playerId)) {
            LOG_WARNING("Invalid token in move request: " + token);
            return buildResponse(ResponseBuilder::unauthorized("invalid token"));
        }

        // 检查玩家是否在游戏中
        auto player = playerManager_->getPlayerById(playerId);
        if (!player) {
            LOG_WARNING("Player not found in move request: " + playerId);
            return buildResponse(ResponseBuilder::notFound("player not in game"));
        }

        // 5. 验证方向
        Direction direction;
        try {
            direction = DirectionUtils::fromString(directionStr);
        } catch (const std::invalid_argument& e) {
            LOG_WARNING("Invalid direction in move request: " + directionStr);
            return buildResponse(ResponseBuilder::badRequest("invalid direction"));
        }

        // 方向不能是 NONE
        if (direction == Direction::NONE) {
            LOG_WARNING("Direction cannot be NONE in move request");
            return buildResponse(ResponseBuilder::badRequest("invalid direction"));
        }

        // 6. 提交移动指令到游戏管理器（GameManager 会检查是否重复提交）
        if (!gameManager_->submitMove(playerId, direction)) {
            LOG_WARNING("Move already submitted this round for player: " + playerId);
            return buildResponse(ResponseBuilder::tooManyRequests(
                "move already submitted this round", 0));
        }

        LOG_DEBUG("Move submitted successfully: Player=" + playerId + 
                 ", Direction=" + directionStr + ", Token=" + token);
        
        return buildResponse(ResponseBuilder::success());
    }
    catch (const std::exception& e) {
        return handleException(e);
    }
}

crow::response RouteHandler::handleLeaderboard(const crow::request& req) {
    try {
        PerformanceMonitor::ScopedRequest metricsGuard("leaderboard");
        const auto& leaderboardConfig = Config::getInstance().getLeaderboard();
        std::string typeStr = "kills";
        int limit = std::min(50, leaderboardConfig.maxEntries);
        int offset = 0;
        long long startTime = 0;
        long long endTime = 0;

        if (const char* typeParam = req.url_params.get("type")) {
            typeStr = typeParam;
        }

        auto parseInt = [](const char* value, int& out) -> bool {
            if (!value) {
                return false;
            }
            try {
                out = std::stoi(value);
                return true;
            } catch (...) {
                return false;
            }
        };

        auto parseInt64 = [](const char* value, long long& out) -> bool {
            if (!value) {
                return false;
            }
            try {
                out = std::stoll(value);
                return true;
            } catch (...) {
                return false;
            }
        };

        if (const char* limitParam = req.url_params.get("limit")) {
            if (!parseInt(limitParam, limit)) {
                return buildResponse(ResponseBuilder::badRequest("invalid limit"));
            }
        }

        if (const char* offsetParam = req.url_params.get("offset")) {
            if (!parseInt(offsetParam, offset)) {
                return buildResponse(ResponseBuilder::badRequest("invalid offset"));
            }
        }

        if (const char* startParam = req.url_params.get("start_time")) {
            if (!parseInt64(startParam, startTime)) {
                return buildResponse(ResponseBuilder::badRequest("invalid start_time"));
            }
        }

        if (const char* endParam = req.url_params.get("end_time")) {
            if (!parseInt64(endParam, endTime)) {
                return buildResponse(ResponseBuilder::badRequest("invalid end_time"));
            }
        }

        std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        LeaderboardType type = LeaderboardType::KILLS;
        if (typeStr == "max_length") {
            type = LeaderboardType::MAX_LENGTH;
        } else if (typeStr != "kills") {
            return buildResponse(ResponseBuilder::badRequest("invalid type"));
        }

        if (limit < 1) {
            limit = 1;
        }
        if (limit > leaderboardConfig.maxEntries) {
            limit = leaderboardConfig.maxEntries;
        }
        if (offset < 0) {
            offset = 0;
        }

        auto entries = leaderboardManager_->getTopPlayers(type, limit, offset, startTime, endTime);

        nlohmann::json entryList = nlohmann::json::array();
        for (const auto& entry : entries) {
            entryList.push_back({
                {"uid", entry.uid},
                {"name", entry.playerName},
                {"season_id", entry.seasonId},
                {"now_length", entry.nowLength},
                {"max_length", entry.maxLength},
                {"kills", entry.kills},
                {"deaths", entry.deaths},
                {"games_played", entry.gamesPlayed},
                {"total_food", entry.totalFood},
                {"last_round", entry.lastRound},
                {"timestamp", entry.timestamp},
                {"rank", entry.rank}
            });
        }

        nlohmann::json data = {
            {"type", typeStr},
            {"limit", limit},
            {"offset", offset},
            {"start_time", startTime},
            {"end_time", endTime},
            {"refresh_interval_rounds", leaderboardConfig.refreshIntervalRounds},
            {"cache_ttl_seconds", leaderboardConfig.cacheTtlSeconds},
            {"entries", entryList}
        };

        return buildResponse(ResponseBuilder::success(data));
    }
    catch (const std::exception& e) {
        return handleException(e);
    }
}

crow::response RouteHandler::handleMetrics(const crow::request& req) {
    try {
        auto& monitor = PerformanceMonitor::getInstance();
        if (!monitor.isEnabled()) {
            return buildResponse(ResponseBuilder::serviceUnavailable("metrics disabled"));
        }

        std::string format = "json";
        if (const char* formatParam = req.url_params.get("format")) {
            format = formatParam;
        }

        std::transform(format.begin(), format.end(), format.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (format == "prometheus") {
            crow::response res;
            res.set_header("Content-Type", "text/plain; version=0.0.4");
            res.body = monitor.toPrometheus();
            res.code = 200;
            return res;
        }

        nlohmann::json data = {
            {"metrics", monitor.toJson()}
        };
        return buildResponse(ResponseBuilder::success(data));
    }
    catch (const std::exception& e) {
        return handleException(e);
    }
}

std::string RouteHandler::getClientIp(const crow::request& req) {
    // 尝试从X-Forwarded-For头获取真实IP
    auto xff_it = req.headers.find("X-Forwarded-For");
    if (xff_it != req.headers.end() && !xff_it->second.empty()) {
        // X-Forwarded-For可能包含多个IP，取第一个
        std::string xff = xff_it->second;
        size_t commaPos = xff.find(',');
        if (commaPos != std::string::npos) {
            xff = xff.substr(0, commaPos);
        }
        // 去除前后空格
        xff.erase(xff.find_last_not_of(" \t") + 1);
        xff.erase(0, xff.find_first_not_of(" \t"));
        if (!xff.empty()) {
            return xff;
        }
    }
    
    // 尝试从X-Real-IP头获取
    auto xri_it = req.headers.find("X-Real-IP");
    if (xri_it != req.headers.end() && !xri_it->second.empty()) {
        return xri_it->second;
    }
    
    // 如果没有代理头，使用远程地址
    // 注意：在Crow中可能需要其他方式获取远程地址
    return req.remote_ip_address;
}

bool RouteHandler::checkRateLimit(const std::string& key, const std::string& endpoint) {
    const auto& rateLimitConfig = Config::getInstance().getRateLimit();
    if (!rateLimitConfig.enabled) {
        return true;
    }
    
    if (endpoint == "status") {
        return rateLimiter_.checkLimit(
            "status:" + key,
            rateLimitConfig.statusPerMinute,
            rateLimitConfig.statusWindowSeconds);
    }
    else if (endpoint == "login") {
        return rateLimiter_.checkLimit(
            "login:" + key,
            rateLimitConfig.loginPerHour,
            rateLimitConfig.loginWindowSeconds);
    }
    else if (endpoint == "join") {
        return rateLimiter_.checkLimit(
            "join:" + key,
            rateLimitConfig.joinPerMinute,
            rateLimitConfig.joinWindowSeconds);
    }
    else if (endpoint == "move") {
        // move端点限制：每回合1次
        int roundTime = Config::getInstance().getGame().roundTimeMs / 1000;
        return rateLimiter_.checkLimit("move:" + key, rateLimitConfig.movePerRound, roundTime);
    }
    else if (endpoint == "map") {
        return rateLimiter_.checkLimit(
            "map:" + key,
            rateLimitConfig.mapPerSecond,
            rateLimitConfig.mapWindowSeconds);
    }
    
    // 默认允许通过
    return true;
}

crow::response RouteHandler::buildResponse(const nlohmann::json& jsonData) {
    crow::response res;
    res.set_header("Content-Type", "application/json");
    res.body = jsonData.dump();
    
    // 根据JSON中的code字段设置HTTP状态码
    if (jsonData.contains("code")) {
        int code = jsonData["code"];
        if (code == 0) {
            res.code = 200;  // 成功
        } else if (code >= 400 && code < 600) {
            res.code = code;  // 使用标准HTTP错误码
        } else {
            res.code = 500;  // 其他错误默认为500
        }
    } else {
        res.code = 200;  // 默认200
    }
    
    return res;
}

crow::response RouteHandler::handleException(const std::exception& e) {
    LOG_ERROR(std::string("Exception: ") + e.what());
    return buildResponse(ResponseBuilder::internalError());
}

} // namespace snake
