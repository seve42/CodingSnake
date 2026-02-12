#include "../include/database/DatabaseManager.h"
#include "../include/utils/Logger.h"
#include <chrono>

namespace snake {

// SQL 表结构定义
static const char* SQL_CREATE_PLAYERS = R"(
CREATE TABLE IF NOT EXISTS players (
    uid TEXT PRIMARY KEY,
    paste TEXT NOT NULL,
    key TEXT UNIQUE NOT NULL,
    created_at INTEGER NOT NULL,
    last_login INTEGER NOT NULL
);
)";

static const char* SQL_CREATE_LEADERBOARD = R"(
CREATE TABLE IF NOT EXISTS leaderboard (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    uid TEXT NOT NULL,
    player_name TEXT NOT NULL,
    season_id TEXT NOT NULL DEFAULT 'all_time',
    season_start INTEGER NOT NULL DEFAULT 0,
    season_end INTEGER NOT NULL DEFAULT 0,
    now_length INTEGER NOT NULL DEFAULT 0,
    max_length INTEGER NOT NULL DEFAULT 0,
    kills INTEGER DEFAULT 0,
    deaths INTEGER DEFAULT 0,
    games_played INTEGER DEFAULT 0,
    total_food INTEGER DEFAULT 0,
    last_round INTEGER NOT NULL DEFAULT 0,
    timestamp INTEGER NOT NULL,
    FOREIGN KEY (uid) REFERENCES players(uid),
    UNIQUE (uid, season_id)
);
)";

static const char* SQL_CREATE_SNAPSHOTS = R"(
CREATE TABLE IF NOT EXISTS game_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    round INTEGER NOT NULL,
    game_state TEXT NOT NULL,
    timestamp INTEGER NOT NULL,
    created_at INTEGER NOT NULL
);
)";

static bool hasTableColumn(DatabaseManager* db, const std::string& table, const std::string& column) {
    auto rs = db->query("PRAGMA table_info(" + table + ");");
    while (rs.next()) {
        if (rs.getString(1) == column) {
            return true;
        }
    }
    return false;
}

DatabaseManager::DatabaseManager()
    : db_(nullptr)
    , connected_(false) {
}

DatabaseManager::~DatabaseManager() {
    close();
}

bool DatabaseManager::initialize(const std::string& dbPath) {
    if (connected_) {
        LOG_WARNING("Database already connected");
        return true;
    }

    // 1. 打开数据库连接
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        lastError_ = "Cannot open database: " + std::string(sqlite3_errmsg(db_));
        LOG_ERROR(lastError_);
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    // 标记为已连接，以便后续的execute可以工作
    connected_ = true;

    // 启用外键约束
    execute("PRAGMA foreign_keys = ON;");

    // 2. 创建表结构
    if (!createTables()) {
        LOG_ERROR("Failed to create tables");
        close();
        return false;
    }

    // 3. 创建索引
    if (!createIndexes()) {
        LOG_ERROR("Failed to create indexes");
        close();
        return false;
    }

    LOG_INFO("Database initialized successfully: " + dbPath);
    return true;
}

bool DatabaseManager::isConnected() const {
    return connected_;
}

void DatabaseManager::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        connected_ = false;
        LOG_INFO("Database connection closed");
    }
}

bool DatabaseManager::execute(const std::string& sql) {
    if (!connected_ || !db_) {
        lastError_ = "Database not connected";
        return false;
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        lastError_ = errMsg ? errMsg : "Unknown error";
        LOG_ERROR("SQL execution failed: " + lastError_);
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

ResultSet DatabaseManager::query(const std::string& sql) {
    ResultSet rs;
    
    if (!connected_ || !db_) {
        lastError_ = "Database not connected";
        return rs;
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        LOG_ERROR("Query preparation failed: " + lastError_);
        return rs;
    }

    rs.stmt_ = stmt;
    return rs;
}

bool DatabaseManager::executeWithParams(const std::string& sql,
                                        const std::vector<std::string>& params) {
    if (!connected_ || !db_) {
        lastError_ = "Database not connected";
        return false;
    }

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) {
        return false;
    }

    if (!bindParameters(stmt, params)) {
        sqlite3_finalize(stmt);
        return false;
    }

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        lastError_ = sqlite3_errmsg(db_);
        LOG_ERROR("SQL execution failed: " + lastError_);
        return false;
    }

    return true;
}

ResultSet DatabaseManager::queryWithParams(const std::string& sql,
                                           const std::vector<std::string>& params) {
    ResultSet rs;
    
    if (!connected_ || !db_) {
        lastError_ = "Database not connected";
        return rs;
    }

    sqlite3_stmt* stmt = prepareStatement(sql);
    if (!stmt) {
        return rs;
    }

    if (!bindParameters(stmt, params)) {
        sqlite3_finalize(stmt);
        return rs;
    }

    rs.stmt_ = stmt;
    return rs;
}

bool DatabaseManager::beginTransaction() {
    return execute("BEGIN TRANSACTION;");
}

bool DatabaseManager::commit() {
    return execute("COMMIT;");
}

bool DatabaseManager::rollback() {
    return execute("ROLLBACK;");
}

long long DatabaseManager::getLastInsertId() {
    if (!connected_ || !db_) {
        return 0;
    }
    return sqlite3_last_insert_rowid(db_);
}

int DatabaseManager::getChangedRowCount() {
    if (!connected_ || !db_) {
        return 0;
    }
    return sqlite3_changes(db_);
}

std::string DatabaseManager::getErrorMessage() const {
    return lastError_;
}

bool DatabaseManager::createTables() {
    if (!execute(SQL_CREATE_PLAYERS)) {
        return false;
    }
    if (!execute(SQL_CREATE_LEADERBOARD)) {
        return false;
    }
    if (!hasTableColumn(this, "leaderboard", "season_id")) {
        if (!execute("ALTER TABLE leaderboard ADD COLUMN season_id TEXT NOT NULL DEFAULT 'all_time';")) {
            return false;
        }
    }
    if (!hasTableColumn(this, "leaderboard", "season_start")) {
        if (!execute("ALTER TABLE leaderboard ADD COLUMN season_start INTEGER NOT NULL DEFAULT 0;")) {
            return false;
        }
    }
    if (!hasTableColumn(this, "leaderboard", "season_end")) {
        if (!execute("ALTER TABLE leaderboard ADD COLUMN season_end INTEGER NOT NULL DEFAULT 0;")) {
            return false;
        }
    }
    if (!hasTableColumn(this, "leaderboard", "now_length")) {
        if (!execute("ALTER TABLE leaderboard ADD COLUMN now_length INTEGER NOT NULL DEFAULT 0;")) {
            return false;
        }
    }
    if (!hasTableColumn(this, "leaderboard", "last_round")) {
        if (!execute("ALTER TABLE leaderboard ADD COLUMN last_round INTEGER NOT NULL DEFAULT 0;")) {
            return false;
        }
    }
    if (!execute(SQL_CREATE_SNAPSHOTS)) {
        return false;
    }
    LOG_INFO("Database tables created successfully");
    return true;
}

bool DatabaseManager::createIndexes() {
    // 创建排行榜索引
    if (!execute("CREATE INDEX IF NOT EXISTS idx_leaderboard_uid ON leaderboard(uid);")) {
        return false;
    }
    if (!execute("CREATE INDEX IF NOT EXISTS idx_leaderboard_season_kills ON leaderboard(season_id, kills DESC);")) {
        return false;
    }
    if (!execute("CREATE INDEX IF NOT EXISTS idx_leaderboard_season_max_length ON leaderboard(season_id, max_length DESC);")) {
        return false;
    }
    if (!execute("CREATE INDEX IF NOT EXISTS idx_leaderboard_uid_season ON leaderboard(uid, season_id);")) {
        return false;
    }
    // 创建快照索引
    if (!execute("CREATE INDEX IF NOT EXISTS idx_snapshots_round ON game_snapshots(round);")) {
        return false;
    }
    if (!execute("CREATE INDEX IF NOT EXISTS idx_snapshots_timestamp ON game_snapshots(timestamp);")) {
        return false;
    }
    LOG_INFO("Database indexes created successfully");
    return true;
}

sqlite3_stmt* DatabaseManager::prepareStatement(const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        LOG_ERROR("Statement preparation failed: " + lastError_);
        return nullptr;
    }
    
    return stmt;
}

bool DatabaseManager::bindParameters(sqlite3_stmt* stmt,
                                     const std::vector<std::string>& params) {
    for (size_t i = 0; i < params.size(); ++i) {
        int rc = sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            lastError_ = sqlite3_errmsg(db_);
            LOG_ERROR("Parameter binding failed: " + lastError_);
            return false;
        }
    }
    return true;
}

// ResultSet 实现
ResultSet::ResultSet()
    : stmt_(nullptr)
    , hasRow_(false) {
}

ResultSet::~ResultSet() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
    }
}

bool ResultSet::next() {
    if (!stmt_) {
        return false;
    }
    
    int rc = sqlite3_step(stmt_);
    
    if (rc == SQLITE_ROW) {
        hasRow_ = true;
        return true;
    } else if (rc == SQLITE_DONE) {
        hasRow_ = false;
        return false;
    } else {
        hasRow_ = false;
        return false;
    }
}

std::string ResultSet::getString(int columnIndex) const {
    if (!stmt_ || !hasRow_) {
        return "";
    }
    
    const unsigned char* text = sqlite3_column_text(stmt_, columnIndex);
    return text ? std::string(reinterpret_cast<const char*>(text)) : "";
}

int ResultSet::getInt(int columnIndex) const {
    if (!stmt_ || !hasRow_) {
        return 0;
    }
    return sqlite3_column_int(stmt_, columnIndex);
}

long long ResultSet::getInt64(int columnIndex) const {
    if (!stmt_ || !hasRow_) {
        return 0;
    }
    return sqlite3_column_int64(stmt_, columnIndex);
}

bool ResultSet::isNull(int columnIndex) const {
    if (!stmt_ || !hasRow_) {
        return true;
    }
    return sqlite3_column_type(stmt_, columnIndex) == SQLITE_NULL;
}

int ResultSet::getColumnCount() const {
    if (!stmt_) {
        return 0;
    }
    return sqlite3_column_count(stmt_);
}

} // namespace snake
