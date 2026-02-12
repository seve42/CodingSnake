# 贪吃蛇服务器架构文档（当前实现）

> 基于 `server/include` 与 `server/src` 当前代码状态整理。

## 1. 总体架构

服务器采用分层模块化结构：

```text
HTTP/Crow 路由层
    ↓
业务管理层（PlayerManager / GameManager / MapManager）
    ↓
数据模型层（Player / Snake / GameState / Food / Direction / Point）
    ↓
持久化层（DatabaseManager / LeaderboardManager / SnapshotManager）
    ↓
SQLite3
```

并行存在的横切能力：

- `Config`：全局配置单例
- `Logger`：日志
- `RateLimiter`：接口限流
- `PerformanceMonitor`：请求、回合、锁等待等指标
- `ResponseBuilder`：统一响应格式
- `Validator`：参数与洛谷验证

---

## 2. 启动流程（main.cpp）

1. 加载 `config.json`（或命令行指定路径）。
2. 初始化日志系统。
3. 初始化并启动 `PerformanceMonitor`。
4. 初始化主数据库连接 `DatabaseManager`。
5. 创建 `LeaderboardManager` 与 `SnapshotManager`。
6. 创建 `MapManager`、`PlayerManager`、`GameManager`。
7. 创建 `RouteHandler` 并注册路由。
8. 启动 `GameManager::start()` 游戏循环线程。
9. 启动 Crow HTTP 服务（端口与线程数来自配置）。

---

## 3. 核心模块职责

## 3.1 RouteHandler（HTTP 层）

路由定义在 `include/handlers/RouteHandler.h`，实现位于 `src/handlers/RouteHandler.cpp`。

当前端点：

- `GET /api/status`
- `POST /api/game/login`
- `POST /api/game/join`
- `GET /api/game/map`
- `GET /api/game/map/delta`
- `POST /api/game/move`
- `GET /api/leaderboard`
- `GET /api/metrics`

特点：

- 请求内统一使用 `ResponseBuilder` 构造 JSON 响应。
- 部分端点带限流（受 `rate_limits.enabled` 控制）。
- 每个端点使用 `PerformanceMonitor::ScopedRequest` 记录延迟。

## 3.2 PlayerManager（认证与会话）

- 登录：`uid + paste` 验证后生成/复用 `key`。
- 加入游戏：`key + name(+color)` → 生成会话 `token` 与 `playerId`。
- 维护内存映射：`key↔uid`、`token→playerId`、`playerId→Player`。

注意：`PlayerManager` 内部会独立创建并初始化一个 `DatabaseManager` 连接。

## 3.3 GameManager（回合驱动核心）

- 独立线程按 `round_time_ms` 推进回合。
- 双缓冲处理移动指令（本回合收集、下回合执行）。
- 维护蛇身占用索引 `occupiedCounts_`，支持 O(1) 级碰撞/食物生成判定。
- 支持增量状态追踪并提供 `getDeltaState()`。
- 在吃食物、击杀、死亡等事件调用 `LeaderboardManager` 更新统计。

## 3.4 MapManager（地图与碰撞）

- 地图边界判断与安全出生点生成。
- 碰撞类型判定（墙、自撞、他蛇）。
- 食物生成：普通版本与高性能 `generateFoodFast()`。

## 3.5 Database/Leaderboard/Snapshot

- `DatabaseManager`：SQLite 连接、建表、索引、参数化 SQL、事务接口。
- `LeaderboardManager`：按回合/死亡/结算增量更新，支持 kills/max_length 排行查询。
- `SnapshotManager`：接口完整，但当前 `src/database/SnapshotManager.cpp` 仍是 `TODO` 占位，尚未形成可用快照链路。

---

## 4. 数据模型与状态组织

`GameState` 保存：

- 当前回合数、时间戳、下一回合时间戳
- 玩家集合（`Player`）
- 食物集合（含 `unordered_set` 与索引加速）
- 增量变化追踪：加入玩家、死亡玩家、食物增删

`Snake` 支持：

- 方向、移动、成长
- 无敌回合
- 自身碰撞判定

---

## 5. 并发与线程安全

- `GameManager`
  - `stateMutex_` 保护全局游戏状态
  - `movesMutex_` 保护移动指令缓冲
  - 游戏线程与 HTTP 请求线程并发访问时通过锁同步
- `PlayerManager`
  - `std::shared_mutex`：读多写少场景优化
- `PerformanceMonitor`
  - 内部互斥保护指标结构，并提供后台日志线程（可配置）

---

## 6. 配置驱动能力

`Config` 单例提供以下配置域：

- `server`：端口、线程数
- `game`：地图尺寸、回合时间、初始长度、无敌回合、食物密度
- `database`：DB 路径、快照/保留、备份参数
- `rate_limits`：端点限流参数
- `auth`：洛谷验证文本
- `leaderboard`：刷新间隔、最大返回条目、缓存 TTL（当前主要用于响应字段）
- `performance_monitor`：采样率、窗口、落盘与滚动配置

---

## 7. 已实现能力与当前边界

### 已实现

- HTTP API 主流程可运行（登录/加入/移动/地图/排行榜/指标）
- 回合推进、碰撞、食物、死亡淘汰
- 排行榜持久化与查询
- 指标采集与 Prometheus 输出

### 边界/待完善

- `SnapshotManager` 未落地（虽已实例化，但核心方法未实现）
- 日志器文件输出等细节仍有待补齐
- 数据库时间戳单位存在历史混用（players 与 leaderboard）

---

## 8. 关键请求数据流（简版）

### 登录

`/api/game/login` → `Validator` → `PlayerManager::login` → `players` 表 → 返回 `key`

### 加入游戏

`/api/game/join` → `PlayerManager::validateKey + join` → `MapManager` 选出生点 → `GameManager::addPlayer` → 返回 `token` 与初始地图

### 移动

`/api/game/move` → `validateToken` → `GameManager::submitMove`（进入当前缓冲）→ 下一 tick 执行移动

### 排行榜

`/api/leaderboard` → `LeaderboardManager::getTopPlayers(...)` → 返回按 kills 或 max_length 排序结果
