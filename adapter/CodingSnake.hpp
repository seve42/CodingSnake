/**
 * CodingSnake.hpp - 贪吃蛇算法竞赛库
 * 
 * 一个极简的、单文件的C++库，让算法竞赛选手用最基础的语法控制贪吃蛇游戏
 * 
 * 使用示例:
 * ```cpp
 * #include "CodingSnake.hpp"
 * 
 * string decide(const GameState& state) {
 *     return "right";
 * }
 * 
 * int main() {
 *     CodingSnake game("http://localhost:18080");
 *     game.login("uid", "paste");
 *     game.join("MyBot");
 *     game.run(decide);
 *     return 0;
 * }
 * ```
 * 
 * @author CodingSnake Team
 * @version 1.0.0
 * @date 2026-02-12
 */

#ifndef CODING_SNAKE_HPP
#define CODING_SNAKE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <exception>
#include <functional>
#include <sstream>
#include <thread>
#include <chrono>

// 第三方依赖库
// 不使用SSL支持，保持简单，避免依赖OpenSSL
#include "third_party/httplib.h"
#include "third_party/json.hpp"

using std::string;
using std::vector;
using std::map;
using std::set;
using std::pair;
using std::cout;
using std::cerr;
using std::endl;
using json = nlohmann::json;

// ============================================================================
// 异常类
// ============================================================================

/**
 * @brief 贪吃蛇库的异常类
 */
class SnakeException : public std::exception {
private:
    string message;
    
public:
    explicit SnakeException(const string& msg) : message(msg) {}
    
    const char* what() const noexcept override {
        return message.c_str();
    }
};

// ============================================================================
// 数据结构
// ============================================================================

/**
 * @brief 二维坐标点
 */
struct Point {
    int x;
    int y;
    
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}
    
    /**
     * @brief 计算与另一点的曼哈顿距离
     */
    int distance(const Point& other) const {
        return std::abs(x - other.x) + std::abs(y - other.y);
    }
    
    /**
     * @brief 计算与另一点的欧氏距离（平方）
     */
    int distanceSquared(const Point& other) const {
        int dx = x - other.x;
        int dy = y - other.y;
        return dx * dx + dy * dy;
    }
    
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
    
    bool operator<(const Point& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

/**
 * @brief 蛇（玩家）
 */
struct Snake {
    string id;                  // 玩家ID
    string name;                // 玩家名称
    string color;               // 蛇的颜色
    Point head;                 // 蛇头位置
    vector<Point> blocks;       // 蛇身所有方块（blocks[0]为头部）
    int length;                 // 蛇的长度
    int invincible_rounds;      // 剩余无敌回合数
    
    Snake() : length(0), invincible_rounds(0) {}
    
    /**
     * @brief 检查某个位置是否在蛇身上
     */
    bool contains(const Point& p) const {
        for (const auto& block : blocks) {
            if (block == p) return true;
        }
        return false;
    }
    
    /**
     * @brief 检查是否处于无敌状态
     */
    bool isInvincible() const {
        return invincible_rounds > 0;
    }
};

// ============================================================================
// 游戏状态类
// ============================================================================

/**
 * @brief 游戏状态，提供给决策函数使用
 */
class GameState {
private:
    map<string, Snake> players_;     // 所有玩家
    set<Point> foods_;               // 所有食物
    string my_id_;                   // 我的ID
    int map_width_;
    int map_height_;
    int current_round_;
    long long next_round_timestamp_;
    
public:
    GameState()
        : map_width_(50), map_height_(50), current_round_(0), next_round_timestamp_(0) {}
    
    /**
     * @brief 设置我的玩家ID
     */
    void setMyId(const string& id) { my_id_ = id; }
    
    /**
     * @brief 设置地图尺寸
     */
    void setMapSize(int width, int height) {
        map_width_ = width;
        map_height_ = height;
    }
    
    /**
     * @brief 设置当前回合数
     */
    void setCurrentRound(int round) { current_round_ = round; }

    /**
     * @brief 设置下一回合时间戳（毫秒）
     */
    void setNextRoundTimestamp(long long ts) { next_round_timestamp_ = ts; }
    
    /**
     * @brief 获取我的蛇
     */
    Snake getMySnake() const {
        auto it = players_.find(my_id_);
        if (it == players_.end()) {
            throw SnakeException("Player not found");
        }
        return it->second;
    }
    
    /**
     * @brief 获取所有玩家（包括自己）
     */
    vector<Snake> getAllPlayers() const {
        vector<Snake> result;
        for (const auto& pair : players_) {
            result.push_back(pair.second);
        }
        return result;
    }
    
    /**
     * @brief 获取其他玩家（不包括自己）
     */
    vector<Snake> getOtherPlayers() const {
        vector<Snake> result;
        for (const auto& pair : players_) {
            if (pair.first != my_id_) {
                result.push_back(pair.second);
            }
        }
        return result;
    }
    
    /**
     * @brief 获取所有食物
     */
    vector<Point> getFoods() const {
        return vector<Point>(foods_.begin(), foods_.end());
    }
    
    /**
     * @brief 获取地图宽度
     */
    int getMapWidth() const { return map_width_; }
    
    /**
     * @brief 获取地图高度
     */
    int getMapHeight() const { return map_height_; }
    
    /**
     * @brief 获取当前回合数
     */
    int getCurrentRound() const { return current_round_; }

    /**
     * @brief 获取下一回合时间戳（毫秒）
     */
    long long getNextRoundTimestamp() const { return next_round_timestamp_; }
    
    /**
     * @brief 检查位置是否在地图内
     */
    bool isValidPos(int x, int y) const {
        return x >= 0 && x < map_width_ && y >= 0 && y < map_height_;
    }
    
    /**
     * @brief 检查位置是否有障碍物（任何玩家的身体）
     */
    bool hasObstacle(int x, int y) const {
        Point p(x, y);
        for (const auto& pair : players_) {
            if (pair.second.contains(p)) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief 根据ID查找玩家
     */
    Snake* findPlayerById(const string& id) {
        auto it = players_.find(id);
        if (it != players_.end()) {
            return &(it->second);
        }
        return nullptr;
    }
    
    /**
     * @brief 清空所有玩家
     */
    void clearPlayers() { players_.clear(); }
    
    /**
     * @brief 添加或更新玩家
     */
    void addOrUpdatePlayer(const Snake& snake) {
        players_[snake.id] = snake;
    }
    
    /**
     * @brief 移除玩家
     */
    void removePlayer(const string& id) {
        players_.erase(id);
    }
    
    /**
     * @brief 清空所有食物
     */
    void clearFoods() { foods_.clear(); }
    
    /**
     * @brief 添加食物
     */
    void addFood(const Point& p) { foods_.insert(p); }
    
    /**
     * @brief 移除食物
     */
    void removeFood(const Point& p) { foods_.erase(p); }
};

// ============================================================================
// 配置结构
// ============================================================================

/**
 * @brief 游戏配置
 */
struct SnakeConfig {
    string server_url;                  // 服务器地址
    int full_map_refresh_rounds;        // 每多少回合刷新完整地图
    int reconnect_attempts;             // 重连尝试次数
    int timeout_ms;                     // 请求超时时间（毫秒）
    bool auto_respawn;                  // 死亡后自动重生
    float respawn_delay_sec;            // 重生延迟（秒）
    bool verbose;                       // 是否输出详细日志
    
    SnakeConfig() 
        : server_url("http://localhost:18080"),
          full_map_refresh_rounds(50),
          reconnect_attempts(3),
          timeout_ms(5000),
          auto_respawn(true),
          respawn_delay_sec(2.0f),
          verbose(false) {}
    
    explicit SnakeConfig(const string& url) : SnakeConfig() {
        server_url = url;
    }
};

// ============================================================================
// 主类：CodingSnake
// ============================================================================

/**
 * @brief 贪吃蛇游戏主类
 */
class CodingSnake {
private:
    SnakeConfig config_;
    GameState state_;
    
    string key_;                // 账号凭证
    string token_;              // 游戏会话凭证
    string player_id_;          // 玩家ID
    string player_name_;        // 玩家名称
    string player_color_;       // 玩家颜色
    
    int round_time_ms_;         // 回合时长（毫秒）
    int last_full_refresh_;     // 上次完整刷新的回合数
    
    bool initialized_;          // 是否已初始化
    bool in_game_;              // 是否在游戏中
    
    // HTTP客户端
    std::unique_ptr<httplib::Client> client_;
    
public:
    /**
     * @brief 构造函数
     * @param url 服务器地址
     */
    explicit CodingSnake(const string& url) 
        : config_(url), round_time_ms_(1000), last_full_refresh_(0),
          initialized_(false), in_game_(false) {
        initHttpClient();
    }
    
    /**
     * @brief 构造函数
     * @param config 配置对象
     */
    explicit CodingSnake(const SnakeConfig& config)
        : config_(config), round_time_ms_(1000), last_full_refresh_(0),
          initialized_(false), in_game_(false) {
        initHttpClient();
    }
    
    /**
     * @brief 设置是否输出详细日志
     */
    void setVerbose(bool verbose) {
        config_.verbose = verbose;
    }
    
    /**
     * @brief 登录获取key
     * @param uid 洛谷用户ID
     * @param paste 洛谷剪贴板后缀
     */
    void login(const string& uid, const string& paste) {
        log("INFO", "正在登录...");
        
        json payload = {
            {"uid", uid},
            {"paste", paste}
        };
        
        auto res = client_->Post("/api/game/login", 
                                  payload.dump(), 
                                  "application/json");
        
        if (!res) {
            throw SnakeException("登录请求失败: 网络错误");
        }
        
        json data = json::parse(res->body);
        
        if (data["code"].get<int>() != 0) {
            throw SnakeException("登录失败: " + data["msg"].get<string>());
        }
        
        key_ = data["data"]["key"].get<string>();
        log("SUCCESS", "✓ 登录成功");
    }
    
    /**
     * @brief 加入游戏
     * @param name 玩家名称
     * @param color 蛇的颜色（可选，默认随机）
     */
    void join(const string& name, const string& color = "") {
        player_name_ = name;
        player_color_ = color.empty() ? generateRandomColor() : color;
        
        joinGameInternal();
        
        // 获取服务器状态
        fetchServerStatus();
        
        initialized_ = true;
    }
    
    /**
     * @brief 运行游戏循环
     * @param decide_func 决策函数，签名为 string(const GameState&)
     */
    void run(std::function<string(const GameState&)> decide_func) {
        if (!initialized_) {
            throw SnakeException("请先调用 login() 和 join()");
        }
        
        log("INFO", "游戏开始！");
        
        int move_count = 0;
        int last_decision_round = -1;
        
        try {
            while (true) {
                // 更新地图状态
                if (!updateMapState()) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }
                
                // 检查是否存活
                if (!in_game_) {
                    if (config_.auto_respawn) {
                        log("WARNING", "已死亡，准备重生...");
                        respawn();
                        last_decision_round = -1;
                        continue;
                    } else {
                        log("INFO", "游戏结束");
                        break;
                    }
                }

                const int current_round = state_.getCurrentRound();
                if (current_round == last_decision_round) {
                    waitForNextRoundWindow();
                    continue;
                }
                
                // 调用用户的决策函数
                string direction;
                try {
                    direction = decide_func(state_);
                } catch (const std::exception& e) {
                    log("ERROR", string("决策函数异常: ") + e.what());
                    direction = "right";  // 默认方向
                }
                
                // 发送移动指令
                if (sendMove(direction)) {
                    move_count++;
                    
                    if (config_.verbose && move_count % 10 == 0) {
                        Snake my = state_.getMySnake();
                        log("INFO", "Round " + std::to_string(state_.getCurrentRound()) +
                            " | Length: " + std::to_string(my.length) +
                            " | Moves: " + std::to_string(move_count));
                    }
                }

                // 每个回合最多决策并提交一次，避免重复提交导致429
                last_decision_round = current_round;
                
                // 根据服务器时间戳对齐到下一回合窗口，避免固定sleep导致时序漂移
                waitForNextRoundWindow();
            }
        } catch (const std::exception& e) {
            log("ERROR", string("游戏循环异常: ") + e.what());
            throw;
        }
    }
    
private:
    /**
     * @brief 初始化HTTP客户端
     */
    void initHttpClient() {
        // 解析URL
        size_t protocol_end = config_.server_url.find("://");
        if (protocol_end == string::npos) {
            throw SnakeException("无效的服务器地址");
        }
        
        string host_port = config_.server_url.substr(protocol_end + 3);
        
        // C++11兼容：使用new + unique_ptr构造
        client_ = std::unique_ptr<httplib::Client>(new httplib::Client(config_.server_url));
        client_->set_connection_timeout(0, config_.timeout_ms * 1000);
        client_->set_read_timeout(config_.timeout_ms / 1000, 0);
        client_->set_write_timeout(config_.timeout_ms / 1000, 0);
    }
    
    /**
     * @brief 加入游戏的内部实现
     */
    void joinGameInternal() {
        log("INFO", "正在加入游戏...");
        
        json payload = {
            {"key", key_},
            {"name", player_name_},
            {"color", player_color_}
        };
        
        auto res = client_->Post("/api/game/join",
                                  payload.dump(),
                                  "application/json");
        
        if (!res) {
            throw SnakeException("加入游戏失败: 网络错误");
        }
        
        json data = json::parse(res->body);
        
        if (data["code"].get<int>() != 0) {
            throw SnakeException("加入游戏失败: " + data["msg"].get<string>());
        }
        
        token_ = data["data"]["token"].get<string>();
        player_id_ = data["data"]["id"].get<string>();
        state_.setMyId(player_id_);
        
        // 初始化地图状态
        if (data["data"].contains("map_state")) {
            parseFullMapState(data["data"]["map_state"]);
            last_full_refresh_ = state_.getCurrentRound();
        }
        
        in_game_ = true;
        log("SUCCESS", "✓ 加入游戏成功 (ID: " + player_id_ + ")");
    }
    
    /**
     * @brief 获取服务器状态
     */
    void fetchServerStatus() {
        auto res = client_->Get("/api/status");
        
        if (!res) {
            log("WARNING", "无法获取服务器状态");
            return;
        }
        
        json data = json::parse(res->body);
        
        if (data["code"].get<int>() == 0) {
            int width = data["data"]["map_size"]["width"].get<int>();
            int height = data["data"]["map_size"]["height"].get<int>();
            round_time_ms_ = data["data"]["round_time"].get<int>();
            
            state_.setMapSize(width, height);
            
            log("INFO", "地图: " + std::to_string(width) + "x" + std::to_string(height) +
                ", 回合: " + std::to_string(round_time_ms_) + "ms");
        }
    }
    
    /**
     * @brief 更新地图状态
     */
    bool updateMapState() {
        // 定期刷新完整地图
        if (state_.getCurrentRound() - last_full_refresh_ >= config_.full_map_refresh_rounds) {
            return fetchFullMap();
        }
        
        // 否则获取增量更新
        return fetchDeltaMap();
    }
    
    /**
     * @brief 获取完整地图
     */
    bool fetchFullMap() {
        auto res = client_->Get("/api/game/map");
        
        if (!res || res->status != 200) {
            return false;
        }
        
        json data = json::parse(res->body);
        
        if (data["code"].get<int>() != 0) {
            return false;
        }
        
        parseFullMapState(data["data"]["map_state"]);
        last_full_refresh_ = state_.getCurrentRound();
        
        return true;
    }
    
    /**
     * @brief 获取增量地图
     */
    bool fetchDeltaMap() {
        auto res = client_->Get("/api/game/map/delta");
        
        if (!res || res->status != 200) {
            return fetchFullMap();  // 失败则回退到完整地图
        }
        
        json data = json::parse(res->body);
        
        if (data["code"].get<int>() != 0) {
            return fetchFullMap();
        }
        
        parseDeltaState(data["data"]["delta_state"]);
        
        return true;
    }
    
    /**
     * @brief 解析完整地图状态
     */
    void parseFullMapState(const json& map_state) {
        state_.setCurrentRound(map_state["round"].get<int>());
        if (map_state.contains("next_round_timestamp")) {
            state_.setNextRoundTimestamp(map_state["next_round_timestamp"].get<long long>());
        }
        
        // 清空并重建玩家列表
        state_.clearPlayers();
        for (const auto& p : map_state["players"]) {
            Snake snake;
            snake.id = p["id"].get<string>();
            snake.name = p["name"].get<string>();
            snake.color = p.value("color", "#FFFFFF");
            snake.head = Point(p["head"]["x"].get<int>(), p["head"]["y"].get<int>());
            snake.length = p["length"].get<int>();
            snake.invincible_rounds = p.value("invincible_rounds", 0);
            
            for (const auto& b : p["blocks"]) {
                snake.blocks.push_back(Point(b["x"].get<int>(), b["y"].get<int>()));
            }

            if (snake.blocks.empty()) {
                snake.blocks.push_back(snake.head);
            }
            
            state_.addOrUpdatePlayer(snake);
        }
        
        // 清空并重建食物列表
        state_.clearFoods();
        for (const auto& f : map_state["foods"]) {
            state_.addFood(Point(f["x"].get<int>(), f["y"].get<int>()));
        }
        
        // 检查自己是否还在游戏中
        in_game_ = (state_.findPlayerById(player_id_) != nullptr);
    }
    
    /**
     * @brief 解析增量状态
     */
    void parseDeltaState(const json& delta) {
        int new_round = delta["round"].get<int>();
        if (delta.contains("next_round_timestamp")) {
            state_.setNextRoundTimestamp(delta["next_round_timestamp"].get<long long>());
        }
        
        // 检查是否丢帧
        if (new_round > state_.getCurrentRound() + 1) {
            log("WARNING", "检测到丢帧，刷新完整地图");
            fetchFullMap();
            return;
        }
        
        state_.setCurrentRound(new_round);
        
        // 移除死亡玩家
        if (delta.contains("died_players")) {
            for (const auto& id : delta["died_players"]) {
                state_.removePlayer(id.get<string>());
            }
        }
        
        // 添加新加入的玩家
        if (delta.contains("joined_players")) {
            for (const auto& p : delta["joined_players"]) {
                Snake snake;
                snake.id = p["id"].get<string>();
                snake.name = p["name"].get<string>();
                snake.color = p.value("color", "#FFFFFF");
                snake.head = Point(p["head"]["x"].get<int>(), p["head"]["y"].get<int>());
                snake.length = p["length"].get<int>();
                snake.invincible_rounds = p.value("invincible_rounds", 0);
                
                for (const auto& b : p["blocks"]) {
                    snake.blocks.push_back(Point(b["x"].get<int>(), b["y"].get<int>()));
                }

                if (snake.blocks.empty()) {
                    snake.blocks.push_back(snake.head);
                }
                
                state_.addOrUpdatePlayer(snake);
            }
        }
        
        // 更新玩家简化信息
        if (delta.contains("players")) {
            for (const auto& p : delta["players"]) {
                string id = p["id"].get<string>();
                Snake* snake = state_.findPlayerById(id);
                
                if (snake) {
                    Point new_head(p["head"]["x"].get<int>(), p["head"]["y"].get<int>());
                    int new_length = p["length"].get<int>();
                    
                    // 更新blocks
                    if (snake->head != new_head) {
                        // 头部移动了
                        snake->blocks.insert(snake->blocks.begin(), new_head);
                        while (static_cast<int>(snake->blocks.size()) > new_length) {
                            snake->blocks.pop_back();
                        }
                    } else if (static_cast<int>(snake->blocks.size()) != new_length) {
                        // 长度变化（吃到食物）
                        if (snake->blocks.empty()) {
                            snake->blocks.push_back(snake->head);
                        }
                        while (static_cast<int>(snake->blocks.size()) < new_length) {
                            snake->blocks.push_back(snake->blocks.back());
                        }
                    }
                    
                    snake->head = new_head;
                    snake->length = new_length;
                    snake->invincible_rounds = p.value("invincible_rounds", 0);
                }
            }
        }
        
        // 移除食物
        if (delta.contains("removed_foods")) {
            for (const auto& f : delta["removed_foods"]) {
                state_.removeFood(Point(f["x"].get<int>(), f["y"].get<int>()));
            }
        }
        
        // 添加食物
        if (delta.contains("added_foods")) {
            for (const auto& f : delta["added_foods"]) {
                state_.addFood(Point(f["x"].get<int>(), f["y"].get<int>()));
            }
        }
        
        // 检查自己是否还在游戏中
        in_game_ = (state_.findPlayerById(player_id_) != nullptr);
    }
    
    /**
     * @brief 发送移动指令
     */
    bool sendMove(const string& direction) {
        json payload = {
            {"token", token_},
            {"direction", direction}
        };
        
        auto res = client_->Post("/api/game/move",
                                  payload.dump(),
                                  "application/json");
        
        if (!res) {
            return false;
        }
        
        json data = json::parse(res->body);
        
        if (data["code"].get<int>() == 404) {
            // 玩家已死亡
            in_game_ = false;
            return false;
        }
        
        return data["code"].get<int>() == 0;
    }

    /**
     * @brief 睡眠到下一回合前的小窗口
     */
    void waitForNextRoundWindow() {
        const long long next_ts = state_.getNextRoundTimestamp();
        const int safety_ms = 15;

        if (next_ts <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(std::max(5, round_time_ms_ / 4)));
            return;
        }

        auto now = std::chrono::system_clock::now();
        long long now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        long long wait_ms = next_ts - now_ms - safety_ms;
        if (wait_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    /**
     * @brief 重生
     */
    void respawn() {
        joinGameInternal();
        
        // 等待无敌时间
        std::this_thread::sleep_for(
            std::chrono::milliseconds(static_cast<int>(config_.respawn_delay_sec * 1000))
        );
    }
    
    /**
     * @brief 生成随机颜色
     */
    string generateRandomColor() {
        const char* colors[] = {
            "#FF0000", "#00FF00", "#0000FF", "#FFFF00",
            "#FF00FF", "#00FFFF", "#FFA500", "#800080",
            "#FFC0CB", "#00D9FF"
        };
        int idx = std::rand() % 10;
        return colors[idx];
    }
    
    /**
     * @brief 日志输出
     */
    void log(const string& level, const string& message) {
        if (!config_.verbose && level != "ERROR" && level != "SUCCESS") {
            return;
        }
        
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
        
        string color_code;
        if (level == "INFO") color_code = "\033[36m";
        else if (level == "SUCCESS") color_code = "\033[32m";
        else if (level == "WARNING") color_code = "\033[33m";
        else if (level == "ERROR") color_code = "\033[31m";
        else color_code = "\033[0m";
        
        cout << color_code << "[" << buffer << "] [" << level << "]\033[0m " 
             << message << endl;
    }
};

#endif // CODING_SNAKE_HPP
