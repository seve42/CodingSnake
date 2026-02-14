#pragma once

#include "../managers/GameManager.h"
#include "../managers/PlayerManager.h"
#include "../managers/MapManager.h"
#include "../database/LeaderboardManager.h"
#include "../utils/RateLimiter.h"
#include "../utils/Logger.h"
#include <crow.h>
#include <crow/middlewares/cors.h>
#include <memory>

namespace snake {

/**
 * @brief HTTP 路由处理器
 * 处理所有 API 端点请求
 */
class RouteHandler {
public:
    RouteHandler(std::shared_ptr<GameManager> gameManager,
                 std::shared_ptr<PlayerManager> playerManager,
                 std::shared_ptr<MapManager> mapManager,
                 std::shared_ptr<LeaderboardManager> leaderboardManager);
    ~RouteHandler();

    // 注册所有路由（支持任何Crow App类型）
    template<typename App>
    void registerRoutes(App& app);

private:
    // API 处理函数
    crow::response handleStatus(const crow::request& req);
    crow::response handleLogin(const crow::request& req);
    crow::response handleJoin(const crow::request& req);
    crow::response handleGetMap(const crow::request& req);
    crow::response handleGetMapDelta(const crow::request& req);
    crow::response handleMove(const crow::request& req);
    crow::response handleLeaderboard(const crow::request& req);
    crow::response handleMetrics(const crow::request& req);

    // 辅助函数
    std::string getClientIp(const crow::request& req);
    bool isLoopbackAddress(const std::string& ip) const;
    bool isLoopbackRequest(const crow::request& req) const;
    bool checkRateLimit(const std::string& key, const std::string& endpoint);
    crow::response buildResponse(const nlohmann::json& jsonData);
    crow::response handleException(const std::exception& e);

    std::shared_ptr<GameManager> gameManager_;
    std::shared_ptr<PlayerManager> playerManager_;
    std::shared_ptr<MapManager> mapManager_;
    std::shared_ptr<LeaderboardManager> leaderboardManager_;
    RateLimiter rateLimiter_;
};

// 模板函数实现必须在头文件中
template<typename App>
void RouteHandler::registerRoutes(App& app) {
    // GET /api/status
    CROW_ROUTE(app, "/api/status")
    ([this](const crow::request& req) {
        return handleStatus(req);
    });

    // POST /api/game/login
    CROW_ROUTE(app, "/api/game/login").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req) {
        return handleLogin(req);
    });

    // POST /api/game/join
    CROW_ROUTE(app, "/api/game/join").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req) {
        return handleJoin(req);
    });

    // GET /api/game/map
    CROW_ROUTE(app, "/api/game/map")
    ([this](const crow::request& req) {
        return handleGetMap(req);
    });

    // GET /api/game/map/delta
    CROW_ROUTE(app, "/api/game/map/delta")
    ([this](const crow::request& req) {
        return handleGetMapDelta(req);
    });

    // POST /api/game/move
    CROW_ROUTE(app, "/api/game/move").methods(crow::HTTPMethod::POST)
    ([this](const crow::request& req) {
        return handleMove(req);
    });

    // GET /api/leaderboard
    CROW_ROUTE(app, "/api/leaderboard")
    ([this](const crow::request& req) {
        return handleLeaderboard(req);
    });

    // GET /api/metrics
    CROW_ROUTE(app, "/api/metrics")
    ([this](const crow::request& req) {
        return handleMetrics(req);
    });

    LOG_INFO("All routes registered");
}

} // namespace snake
