# 数据库实现文档（与当前代码一致）

> 更新时间：基于 `server/src` 与 `server/include` 当前实现（SQLite3）

## 1. 概览

- 数据库类型：SQLite3
- 默认路径：`./data/snake.db`（来自 `config.json` 的 `database.path`）
- 初始化入口：`DatabaseManager::initialize()`
- 主要用途：
  - 玩家账号认证（`players`）
  - 排行榜统计（`leaderboard`）
  - 游戏快照存储表（`game_snapshots`，表已建，但管理器功能尚未实现）

---

## 2. 当前表结构

以下 SQL 来自 `src/database/DatabaseManager.cpp`。

### 2.1 players

```sql
CREATE TABLE IF NOT EXISTS players (
    uid TEXT PRIMARY KEY,
    paste TEXT NOT NULL,
    key TEXT UNIQUE NOT NULL,
    created_at INTEGER NOT NULL,
    last_login INTEGER NOT NULL
);
```

用途：保存登录认证信息（UID、paste、key）。

### 2.2 leaderboard

```sql
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
```

用途：保存玩家在赛季维度的累计表现（默认赛季 `all_time`）。

### 2.3 game_snapshots

```sql
CREATE TABLE IF NOT EXISTS game_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    round INTEGER NOT NULL,
    game_state TEXT NOT NULL,
    timestamp INTEGER NOT NULL,
    created_at INTEGER NOT NULL
);
```

用途：为快照能力预留存储；当前管理器方法大多仍为 `TODO` 占位实现。

---

## 3. 初始化与迁移策略

`createTables()` 除了建表外，还会对旧版 `leaderboard` 自动补列：

- `season_id`
- `season_start`
- `season_end`
- `now_length`
- `last_round`

实现方式：通过 `PRAGMA table_info(...)` 检查后执行 `ALTER TABLE ... ADD COLUMN ...`。

---

## 4. 当前索引

来自 `createIndexes()`：

```sql
CREATE INDEX IF NOT EXISTS idx_leaderboard_uid ON leaderboard(uid);
CREATE INDEX IF NOT EXISTS idx_leaderboard_season_kills ON leaderboard(season_id, kills DESC);
CREATE INDEX IF NOT EXISTS idx_leaderboard_season_max_length ON leaderboard(season_id, max_length DESC);
CREATE INDEX IF NOT EXISTS idx_leaderboard_uid_season ON leaderboard(uid, season_id);

CREATE INDEX IF NOT EXISTS idx_snapshots_round ON game_snapshots(round);
CREATE INDEX IF NOT EXISTS idx_snapshots_timestamp ON game_snapshots(timestamp);
```

说明：当前代码**没有**创建 `idx_snapshots_created_at`。

---

## 5. 业务数据写入路径（现状）

### 5.1 登录与账号（PlayerManager）

1. `login(uid, paste)` 先走 `Validator::validateLuoguPaste`。
2. 查询 `players`：
   - 若存在且 `paste` 相同：更新 `last_login`，复用旧 `key`。
   - 若存在但 `paste` 变化：生成新 `key` 并覆盖旧值。
   - 若不存在：插入新行。

### 5.2 排行榜（LeaderboardManager）

由 `GameManager` 在关键时机调用：

- `updateOnRound(...)`：吃食物、击杀等回合内增量
- `updateOnDeath(...)`：死亡时更新
- `updateOnGameEnd(...)`：预留接口（当前流程未明显使用）

读取接口主要是：

- `getTopPlayersByKills(...)`
- `getTopPlayersByMaxLength(...)`
- `getTopPlayers(type, limit, offset, startTime, endTime)`

### 5.3 快照（SnapshotManager）

- 表已存在，`main.cpp` 也已实例化 `SnapshotManager`。
- 但 `save/load/clean/replay` 等方法当前都返回默认值（`TODO`），尚未接入实际游戏循环。

---

## 6. 时间戳约定（重要）

当前实现存在单位混用，需要在后续统一：

- `LeaderboardManager` 使用毫秒时间戳（`duration_cast<milliseconds>`）。
- `PlayerManager` 在写 `players.created_at/last_login` 时使用 `system_clock::now().time_since_epoch().count()`，通常是纳秒或实现相关单位。

建议：统一为毫秒级 Unix 时间戳，避免跨表比较和运维脚本歧义。

---

## 7. 已实现 / 未实现边界

### 已实现

- SQLite 连接、建表、建索引、参数化查询、事务接口
- 玩家认证数据持久化
- 排行榜增量更新与查询（按 `kills` / `max_length`）

### 未完全实现

- `SnapshotManager` 全量功能（保存、加载、回放、清理）
- 备份策略虽有配置项（`backup_*`），当前代码未看到完整调度实现
