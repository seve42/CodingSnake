/**
 * CodingSnake.hpp - Snake Algorithm Competition Library
 * 
 * A minimal single-header C++ library for controlling snake bots with basic syntax.
 * 
 * Usage example:
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

// Third-party dependencies
// SSL is intentionally disabled to keep setup simple and avoid OpenSSL dependency
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
// Exception class
// ============================================================================

/**
 * @brief Exception type for the CodingSnake library.
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
// Data structures
// ============================================================================

/**
 * @brief 2D coordinate point.
 */
struct Point {
    int x;
    int y;
    
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}
    
    /**
    * @brief Manhattan distance to another point.
     */
    int distance(const Point& other) const {
        return std::abs(x - other.x) + std::abs(y - other.y);
    }
    
    /**
    * @brief Squared Euclidean distance to another point.
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
 * @brief Snake (player).
 */
struct Snake {
    string id;                  // Player ID
    string name;                // Player name
    string color;               // Snake color
    Point head;                 // Head position
    vector<Point> blocks;       // All snake blocks (blocks[0] is the head)
    int length;                 // Snake length
    int invincible_rounds;      // Remaining invincible rounds
    
    Snake() : length(0), invincible_rounds(0) {}
    
    /**
    * @brief Check whether a position is on the snake body.
     */
    bool contains(const Point& p) const {
        for (const auto& block : blocks) {
            if (block == p) return true;
        }
        return false;
    }
    
    /**
    * @brief Check whether the snake is invincible.
     */
    bool isInvincible() const {
        return invincible_rounds > 0;
    }
};

// ============================================================================
// Game state class
// ============================================================================

/**
 * @brief Game state passed to the decision function.
 */
class GameState {
private:
    map<string, Snake> players_;     // All players
    set<Point> foods_;               // All foods
    string my_id_;                   // My player ID
    int map_width_;
    int map_height_;
    int current_round_;
    long long next_round_timestamp_;
    
public:
    GameState()
        : map_width_(50), map_height_(50), current_round_(0), next_round_timestamp_(0) {}
    
    /**
    * @brief Set my player ID.
     */
    void setMyId(const string& id) { my_id_ = id; }
    
    /**
    * @brief Set map size.
     */
    void setMapSize(int width, int height) {
        map_width_ = width;
        map_height_ = height;
    }
    
    /**
    * @brief Set current round.
     */
    void setCurrentRound(int round) { current_round_ = round; }

    /**
    * @brief Set next round timestamp (ms).
     */
    void setNextRoundTimestamp(long long ts) { next_round_timestamp_ = ts; }
    
    /**
    * @brief Get my snake.
     */
    Snake getMySnake() const {
        auto it = players_.find(my_id_);
        if (it == players_.end()) {
            throw SnakeException("Player not found");
        }
        return it->second;
    }
    
    /**
    * @brief Get all players (including self).
     */
    vector<Snake> getAllPlayers() const {
        vector<Snake> result;
        for (const auto& pair : players_) {
            result.push_back(pair.second);
        }
        return result;
    }
    
    /**
    * @brief Get other players (excluding self).
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
    * @brief Get all foods.
     */
    vector<Point> getFoods() const {
        return vector<Point>(foods_.begin(), foods_.end());
    }
    
    /**
    * @brief Get map width.
     */
    int getMapWidth() const { return map_width_; }
    
    /**
    * @brief Get map height.
     */
    int getMapHeight() const { return map_height_; }
    
    /**
    * @brief Get current round.
     */
    int getCurrentRound() const { return current_round_; }

    /**
    * @brief Get next round timestamp (ms).
     */
    long long getNextRoundTimestamp() const { return next_round_timestamp_; }
    
    /**
    * @brief Check whether a position is inside map bounds.
     */
    bool isValidPos(int x, int y) const {
        return x >= 0 && x < map_width_ && y >= 0 && y < map_height_;
    }
    
    /**
    * @brief Check whether a position has an obstacle (any snake body block).
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
    * @brief Find player by ID.
     */
    Snake* findPlayerById(const string& id) {
        auto it = players_.find(id);
        if (it != players_.end()) {
            return &(it->second);
        }
        return nullptr;
    }
    
    /**
    * @brief Clear all players.
     */
    void clearPlayers() { players_.clear(); }
    
    /**
    * @brief Add or update a player.
     */
    void addOrUpdatePlayer(const Snake& snake) {
        players_[snake.id] = snake;
    }
    
    /**
    * @brief Remove a player.
     */
    void removePlayer(const string& id) {
        players_.erase(id);
    }
    
    /**
    * @brief Clear all foods.
     */
    void clearFoods() { foods_.clear(); }
    
    /**
    * @brief Add food.
     */
    void addFood(const Point& p) { foods_.insert(p); }
    
    /**
    * @brief Remove food.
     */
    void removeFood(const Point& p) { foods_.erase(p); }
};

// ============================================================================
// Config struct
// ============================================================================

/**
 * @brief Game configuration.
 */
struct SnakeConfig {
    string server_url;                  // Server URL
    int full_map_refresh_rounds;        // Full map refresh interval (rounds)
    int reconnect_attempts;             // Reconnect attempts
    int timeout_ms;                     // Request timeout (ms)
    bool auto_respawn;                  // Auto respawn after death
    float respawn_delay_sec;            // Respawn delay (seconds)
    bool verbose;                       // Enable verbose log
    
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
// Main class: CodingSnake
// ============================================================================

/**
 * @brief Main class for snake gameplay.
 */
class CodingSnake {
private:
    SnakeConfig config_;
    GameState state_;
    
    string key_;                // Account credential
    string token_;              // Game session credential
    string player_id_;          // Player ID
    string player_name_;        // Player name
    string player_color_;       // Player color
    
    int round_time_ms_;         // Round duration (ms)
    int last_full_refresh_;     // Round number of last full refresh
    
    bool initialized_;          // Whether initialized
    bool in_game_;              // Whether currently in game
    
    // HTTP client
    std::unique_ptr<httplib::Client> client_;
    
public:
    /**
    * @brief Constructor.
    * @param url Server URL.
     */
    explicit CodingSnake(const string& url) 
        : config_(url), round_time_ms_(1000), last_full_refresh_(0),
          initialized_(false), in_game_(false) {
        initHttpClient();
    }
    
    /**
    * @brief Constructor.
    * @param config Config object.
     */
    explicit CodingSnake(const SnakeConfig& config)
        : config_(config), round_time_ms_(1000), last_full_refresh_(0),
          initialized_(false), in_game_(false) {
        initHttpClient();
    }
    
    /**
    * @brief Set verbose logging.
     */
    void setVerbose(bool verbose) {
        config_.verbose = verbose;
    }
    
    /**
    * @brief Log in and get key.
    * @param uid Luogu user ID.
    * @param paste Luogu clipboard suffix.
     */
    void login(const string& uid, const string& paste) {
        log("INFO", "Logging in...");
        
        json payload = {
            {"uid", uid},
            {"paste", paste}
        };
        
        auto res = client_->Post("/api/game/login", 
                                  payload.dump(), 
                                  "application/json");
        
        if (!res) {
            throw SnakeException("Login request failed: network error");
        }
        
        json data = json::parse(res->body);
        
        if (data["code"].get<int>() != 0) {
            throw SnakeException("Login failed: " + data["msg"].get<string>());
        }
        
        key_ = data["data"]["key"].get<string>();
        log("SUCCESS", "ok Login successful");
    }
    
    /**
    * @brief Join game.
    * @param name Player name.
    * @param color Snake color (optional, random by default).
     */
    void join(const string& name, const string& color = "") {
        player_name_ = name;
        player_color_ = color.empty() ? generateRandomColor() : color;
        
        joinGameInternal();
        
        // Fetch server status
        fetchServerStatus();
        
        initialized_ = true;
    }
    
    /**
    * @brief Run game loop.
    * @param decide_func Decision function with signature string(const GameState&).
     */
    void run(std::function<string(const GameState&)> decide_func) {
        if (!initialized_) {
            throw SnakeException("Please call login() and join() first");
        }
        
        log("INFO", "Game started!");
        
        int move_count = 0;
        int last_decision_round = -1;
        
        try {
            while (true) {
                // Update map state
                if (!updateMapState()) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }
                
                // Check if still alive
                if (!in_game_) {
                    if (config_.auto_respawn) {
                        log("WARNING", "Dead, preparing to respawn...");
                        respawn();
                        last_decision_round = -1;
                        continue;
                    } else {
                        log("INFO", "Game over");
                        break;
                    }
                }

                const int current_round = state_.getCurrentRound();
                if (current_round == last_decision_round) {
                    waitForNextRoundWindow();
                    continue;
                }
                
                // Call user decision function
                string direction;
                try {
                    direction = decide_func(state_);
                } catch (const std::exception& e) {
                    log("ERROR", string("Decision function error: ") + e.what());
                    direction = "right";  // Default direction
                }
                
                // Send move command
                if (sendMove(direction)) {
                    move_count++;
                    
                    if (config_.verbose && move_count % 10 == 0) {
                        Snake my = state_.getMySnake();
                        log("INFO", "Round " + std::to_string(state_.getCurrentRound()) +
                            " | Length: " + std::to_string(my.length) +
                            " | Moves: " + std::to_string(move_count));
                    }
                }

                // At most one decision submission per round to avoid repeated 429 errors
                last_decision_round = current_round;
                
                // Align with next-round window by server timestamp to avoid sleep drift
                waitForNextRoundWindow();
            }
        } catch (const std::exception& e) {
            log("ERROR", string("Game loop error: ") + e.what());
            throw;
        }
    }
    
private:
    /**
    * @brief Initialize HTTP client.
     */
    void initHttpClient() {
        // Parse URL
        size_t protocol_end = config_.server_url.find("://");
        if (protocol_end == string::npos) {
            throw SnakeException("Invalid server URL");
        }
        
        string host_port = config_.server_url.substr(protocol_end + 3);
        
        // C++11 compatible: construct with new + unique_ptr
        client_ = std::unique_ptr<httplib::Client>(new httplib::Client(config_.server_url));
        client_->set_connection_timeout(0, config_.timeout_ms * 1000);
        client_->set_read_timeout(config_.timeout_ms / 1000, 0);
        client_->set_write_timeout(config_.timeout_ms / 1000, 0);
    }
    
    /**
    * @brief Internal implementation of join game.
     */
    void joinGameInternal() {
        log("INFO", "Joining game...");
        
        json payload = {
            {"key", key_},
            {"name", player_name_},
            {"color", player_color_}
        };
        
        auto res = client_->Post("/api/game/join",
                                  payload.dump(),
                                  "application/json");
        
        if (!res) {
            throw SnakeException("Join game failed: network error");
        }
        
        json data = json::parse(res->body);
        
        if (data["code"].get<int>() != 0) {
            throw SnakeException("Join game failed: " + data["msg"].get<string>());
        }
        
        token_ = data["data"]["token"].get<string>();
        player_id_ = data["data"]["id"].get<string>();
        state_.setMyId(player_id_);
        
        // Initialize map state
        if (data["data"].contains("map_state")) {
            parseFullMapState(data["data"]["map_state"]);
            last_full_refresh_ = state_.getCurrentRound();
        }
        
        in_game_ = true;
        log("SUCCESS", "ok Joined game successfully (ID: " + player_id_ + ")");
    }
    
    /**
    * @brief Fetch server status.
     */
    void fetchServerStatus() {
        auto res = client_->Get("/api/status");
        
        if (!res) {
            log("WARNING", "Unable to fetch server status");
            return;
        }
        
        json data = json::parse(res->body);
        
        if (data["code"].get<int>() == 0) {
            int width = data["data"]["map_size"]["width"].get<int>();
            int height = data["data"]["map_size"]["height"].get<int>();
            round_time_ms_ = data["data"]["round_time"].get<int>();
            
            state_.setMapSize(width, height);
            
            log("INFO", "Map: " + std::to_string(width) + "x" + std::to_string(height) +
                ", Round: " + std::to_string(round_time_ms_) + "ms");
        }
    }
    
    /**
    * @brief Update map state.
     */
    bool updateMapState() {
        // Periodically refresh full map
        if (state_.getCurrentRound() - last_full_refresh_ >= config_.full_map_refresh_rounds) {
            return fetchFullMap();
        }
        
        // Otherwise fetch delta update
        return fetchDeltaMap();
    }
    
    /**
    * @brief Fetch full map.
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
    * @brief Fetch delta map.
     */
    bool fetchDeltaMap() {
        auto res = client_->Get("/api/game/map/delta");
        
        if (!res || res->status != 200) {
            return fetchFullMap();  // Fallback to full map on failure
        }
        
        json data = json::parse(res->body);
        
        if (data["code"].get<int>() != 0) {
            return fetchFullMap();
        }
        
        parseDeltaState(data["data"]["delta_state"]);
        
        return true;
    }
    
    /**
    * @brief Parse full map state.
     */
    void parseFullMapState(const json& map_state) {
        state_.setCurrentRound(map_state["round"].get<int>());
        if (map_state.contains("next_round_timestamp")) {
            state_.setNextRoundTimestamp(map_state["next_round_timestamp"].get<long long>());
        }
        
        // Clear and rebuild players
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
        
        // Clear and rebuild foods
        state_.clearFoods();
        for (const auto& f : map_state["foods"]) {
            state_.addFood(Point(f["x"].get<int>(), f["y"].get<int>()));
        }
        
        // Check whether self is still in game
        in_game_ = (state_.findPlayerById(player_id_) != nullptr);
    }
    
    /**
    * @brief Parse delta state.
     */
    void parseDeltaState(const json& delta) {
        int new_round = delta["round"].get<int>();
        if (delta.contains("next_round_timestamp")) {
            state_.setNextRoundTimestamp(delta["next_round_timestamp"].get<long long>());
        }
        
        // Check for dropped frames
        if (new_round > state_.getCurrentRound() + 1) {
            log("WARNING", "Frame drop detected, refreshing full map");
            fetchFullMap();
            return;
        }
        
        state_.setCurrentRound(new_round);
        
        // Remove dead players
        if (delta.contains("died_players")) {
            for (const auto& id : delta["died_players"]) {
                state_.removePlayer(id.get<string>());
            }
        }
        
        // Add newly joined players
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
        
        // Update simplified player info
        if (delta.contains("players")) {
            for (const auto& p : delta["players"]) {
                string id = p["id"].get<string>();
                Snake* snake = state_.findPlayerById(id);
                
                if (snake) {
                    Point new_head(p["head"]["x"].get<int>(), p["head"]["y"].get<int>());
                    int new_length = p["length"].get<int>();
                    
                    // Update blocks
                    if (snake->head != new_head) {
                        // Head moved
                        snake->blocks.insert(snake->blocks.begin(), new_head);
                        while (static_cast<int>(snake->blocks.size()) > new_length) {
                            snake->blocks.pop_back();
                        }
                    } else if (static_cast<int>(snake->blocks.size()) != new_length) {
                        // Length changed (food eaten)
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
        
        // Remove foods
        if (delta.contains("removed_foods")) {
            for (const auto& f : delta["removed_foods"]) {
                state_.removeFood(Point(f["x"].get<int>(), f["y"].get<int>()));
            }
        }
        
        // Add foods
        if (delta.contains("added_foods")) {
            for (const auto& f : delta["added_foods"]) {
                state_.addFood(Point(f["x"].get<int>(), f["y"].get<int>()));
            }
        }
        
        // Check whether self is still in game
        in_game_ = (state_.findPlayerById(player_id_) != nullptr);
    }
    
    /**
    * @brief Send move command.
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
            // Player is dead
            in_game_ = false;
            return false;
        }
        
        return data["code"].get<int>() == 0;
    }

    /**
    * @brief Sleep until the pre-next-round time window.
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
    * @brief Respawn.
     */
    void respawn() {
        joinGameInternal();
        
        // Wait for invincibility duration
        std::this_thread::sleep_for(
            std::chrono::milliseconds(static_cast<int>(config_.respawn_delay_sec * 1000))
        );
    }
    
    /**
    * @brief Generate random color.
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
    * @brief Log output.
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
