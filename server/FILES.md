# 项目文件清单（当前版本）

> 本文档按 `server/` 当前代码结构整理，不再使用旧的占位说明。

## 1. 顶层文件

- `README.md`：项目说明与快速使用
- `ARCHITECTURE.md`：架构设计与模块关系
- `DATABASE.md`：数据库实现文档
- `FILES.md`：当前文件清单（本文件）
- `CMakeLists.txt`：构建配置（Crow/json 通过 FetchContent）
- `config.json`：运行配置（服务、游戏、数据库、限流、排行榜、性能监控）
- `play_demo.py`：本地演示脚本
- `.gitignore`：忽略规则

## 2. 目录结构

```text
server/
├── include/
│   ├── models/
│   ├── managers/
│   ├── database/
│   ├── handlers/
│   └── utils/
├── src/
│   ├── models/
│   ├── managers/
│   ├── database/
│   ├── handlers/
│   └── utils/
├── data/
│   ├── README.md
│   ├── snake.db
│   └── metrics.log
└── build/
```

---

## 3. include/ 头文件（19）

### 3.1 models

- `include/models/Point.h`
- `include/models/Direction.h`
- `include/models/Snake.h`
- `include/models/Player.h`
- `include/models/Food.h`
- `include/models/GameState.h`
- `include/models/Config.h`

### 3.2 managers

- `include/managers/PlayerManager.h`
- `include/managers/GameManager.h`
- `include/managers/MapManager.h`

### 3.3 database

- `include/database/DatabaseManager.h`
- `include/database/LeaderboardManager.h`
- `include/database/SnapshotManager.h`

### 3.4 handlers

- `include/handlers/RouteHandler.h`

### 3.5 utils

- `include/utils/Logger.h`
- `include/utils/RateLimiter.h`
- `include/utils/ResponseBuilder.h`
- `include/utils/Validator.h`
- `include/utils/PerformanceMonitor.h`

---

## 4. src/ 源文件

### 4.1 程序入口

- `src/main.cpp`

### 4.2 models（实现）

- `src/models/Point.cpp`
- `src/models/Direction.cpp`
- `src/models/Snake.cpp`
- `src/models/Player.cpp`
- `src/models/Food.cpp`
- `src/models/GameState.cpp`
- `src/models/Config.cpp`
- `src/models/README_SNAKE.md`

### 4.3 managers（实现）

- `src/managers/PlayerManager.cpp`
- `src/managers/GameManager.cpp`
- `src/managers/MapManager.cpp`

### 4.4 database（实现）

- `src/database/DatabaseManager.cpp`
- `src/database/LeaderboardManager.cpp`
- `src/database/SnapshotManager.cpp`

### 4.5 handlers（实现）

- `src/handlers/RouteHandler.cpp`

### 4.6 utils（实现）

- `src/utils/Logger.cpp`
- `src/utils/RateLimiter.cpp`
- `src/utils/ResponseBuilder.cpp`
- `src/utils/Validator.cpp`
- `src/utils/PerformanceMonitor.cpp`

---

## 5. 当前实现状态（按代码）

- 已形成完整可运行后端：配置加载、路由注册、游戏循环、排行榜更新、基础持久化。
- `PerformanceMonitor` 已接入请求与回合指标，并支持 JSON/Prometheus 输出。
- `SnapshotManager` 接口齐全，但 `src/database/SnapshotManager.cpp` 仍为 `TODO` 占位实现。
- 日志器部分能力（如文件写入细节）仍有待补全。

---

## 6. 文件数量速览

- 头文件（`include/`）：19
- C++ 源文件（`src/**/*.cpp`）：20
- 代码内附加文档（`src/models/README_SNAKE.md`）：1
- 顶层文档（`*.md`）：4
