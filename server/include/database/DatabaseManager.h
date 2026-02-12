#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <sqlite3.h>

namespace snake {

/**
 * @brief 数据库查询结果集
 */
class ResultSet {
public:
    ResultSet();
    ~ResultSet();

    bool next();
    std::string getString(int columnIndex) const;
    int getInt(int columnIndex) const;
    long long getInt64(int columnIndex) const;
    bool isNull(int columnIndex) const;
    int getColumnCount() const;

private:
    friend class DatabaseManager;
    sqlite3_stmt* stmt_;
    bool hasRow_;
};

/**
 * @brief 数据库管理器
 * 负责 SQLite 数据库的连接、初始化和基本操作
 */
class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    // 初始化和连接
    bool initialize(const std::string& dbPath);
    bool isConnected() const;
    void close();

    // 基本操作
    bool execute(const std::string& sql);
    ResultSet query(const std::string& sql);

    // 参数化查询（防止 SQL 注入）
    bool executeWithParams(const std::string& sql, 
                          const std::vector<std::string>& params);
    ResultSet queryWithParams(const std::string& sql,
                             const std::vector<std::string>& params);

    // 事务管理
    bool beginTransaction();
    bool commit();
    bool rollback();

    // 工具方法
    long long getLastInsertId();
    int getChangedRowCount();
    std::string getErrorMessage() const;

private:
    bool createTables();
    bool createIndexes();
    sqlite3_stmt* prepareStatement(const std::string& sql);
    bool bindParameters(sqlite3_stmt* stmt, 
                       const std::vector<std::string>& params);

    sqlite3* db_;
    std::mutex mutex_;
    bool connected_;
    std::string lastError_;
};

} // namespace snake
