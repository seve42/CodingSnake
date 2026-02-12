#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <memory>

#include "models/Config.h"
#include "managers/GameManager.h"
#include "managers/PlayerManager.h"
#include "managers/MapManager.h"
#include "database/DatabaseManager.h"
#include "database/LeaderboardManager.h"
#include "database/SnapshotManager.h"
#include "handlers/RouteHandler.h"
#include "utils/Logger.h"
#include "utils/PerformanceMonitor.h"

int main(int argc, char* argv[]) {
    using namespace snake;

    // 加载配置
    auto& config = Config::getInstance();
    std::string configFile = "config.json";
    if (argc > 1) {
        configFile = argv[1];
    }
    
    if (!config.loadFromFile(configFile)) {
        std::cerr << "Failed to load config file: " << configFile << std::endl;
        std::cerr << "Using default configuration..." << std::endl;
    }

    // 初始化日志系统
    Logger::getInstance().setLevel(Logger::Level::INFO);
    Logger::getInstance().enableConsole(true);
    LOG_INFO("Snake Game Server initializing...");

    // 初始化性能监控
    {
        const auto& perfConfig = config.getPerformanceMonitor();
        PerformanceMonitor::Config monitorConfig;
        monitorConfig.enabled = perfConfig.enabled;
        monitorConfig.sampleRate = perfConfig.sampleRate;
        monitorConfig.windowSeconds = perfConfig.windowSeconds;
        monitorConfig.maxSamples = perfConfig.maxSamples;
        monitorConfig.logEnabled = perfConfig.logEnabled;
        monitorConfig.logIntervalSeconds = perfConfig.logIntervalSeconds;
        monitorConfig.logPath = perfConfig.logPath;
        monitorConfig.logMaxBytes = perfConfig.logMaxBytes;
        monitorConfig.logMaxFiles = perfConfig.logMaxFiles;
        PerformanceMonitor::getInstance().configure(monitorConfig);
        PerformanceMonitor::getInstance().start();
    }

    // 初始化数据库
    auto dbManager = std::make_shared<DatabaseManager>();
    if (!dbManager->initialize(config.getDatabase().path)) {
        LOG_ERROR("Failed to initialize database");
        return 1;
    }
    LOG_INFO("Database initialized successfully");

    // 创建数据库管理器
    auto leaderboardManager = std::make_shared<LeaderboardManager>(dbManager);
    auto snapshotManager = std::make_shared<SnapshotManager>(dbManager);

    // 创建管理器
    auto mapManager = std::make_shared<MapManager>(
        config.getGame().mapWidth,
        config.getGame().mapHeight
    );
    
    auto playerManager = std::make_shared<PlayerManager>();
    
    auto gameManager = std::make_shared<GameManager>(
        mapManager,
        playerManager,
        leaderboardManager
    );

    // 创建路由处理器
    auto routeHandler = std::make_shared<RouteHandler>(
        gameManager,
        playerManager,
        mapManager,
        leaderboardManager
    );

    // 创建并配置 Crow 应用（启用CORS支持）
    crow::App<crow::CORSHandler> app;
    
    // 配置CORS，允许所有来源
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("*")
        .methods("GET"_method, "POST"_method, "OPTIONS"_method)
        .headers("Content-Type", "Accept");
    
    // 注册所有路由
    routeHandler->registerRoutes(app);

    // 启动游戏循环
    gameManager->start();
    LOG_INFO("Game loop started");

    // 启动 HTTP 服务器
    const int port = config.getServer().port;
    const int threads = config.getServer().threads;
    
    LOG_INFO("Server starting on port " + std::to_string(port) + 
             " with " + std::to_string(threads) + " threads...");
    
    app.port(port).multithreaded().concurrency(threads).run();

    // 关闭游戏循环
    gameManager->stop();
    LOG_INFO("Server shutdown complete");

    // 关闭性能监控
    PerformanceMonitor::getInstance().stop();

    return 0;
}
