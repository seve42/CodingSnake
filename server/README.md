# 贪吃蛇多人对战服务器

基于 C++ 和 Crow 框架的多人在线贪吃蛇游戏服务器。

## 项目结构

```
server/
├── ARCHITECTURE.md          # 架构设计文档
├── README.md               # 本文件
├── CMakeLists.txt          # CMake 构建配置
├── config.json             # 服务器配置文件
│
├── include/                # 头文件目录
│   ├── models/            # 数据模型
│   │   ├── Point.h           - 坐标点
│   │   ├── Direction.h       - 移动方向
│   │   ├── Snake.h           - 蛇
│   │   ├── Player.h          - 玩家
│   │   ├── Food.h            - 食物
│   │   ├── GameState.h       - 游戏状态
│   │   └── Config.h          - 配置管理
│   │
│   ├── managers/          # 业务逻辑管理器
│   │   ├── PlayerManager.h   - 玩家管理（认证、会话）
│   │   ├── GameManager.h     - 游戏管理（回合、规则）
│   │   └── MapManager.h      - 地图管理（碰撞、食物）
│   │
│   ├── database/          # 数据库管理
│   │   ├── DatabaseManager.h - 数据库连接和操作
│   │   ├── LeaderboardManager.h - 排行榜管理
│   │   └── SnapshotManager.h - 游戏快照管理
│   │
│   ├── handlers/          # HTTP 路由处理
│   │   └── RouteHandler.h    - API 端点处理
│   │
│   └── utils/             # 工具类
│       ├── RateLimiter.h     - 速率限制
│       ├── Logger.h          - 日志系统
│       ├── ResponseBuilder.h - 响应构造
│       └── Validator.h       - 输入验证
│
├── src/                   # 源文件目录
│   ├── main.cpp           # 主程序入口
│   ├── managers/          # 管理器实现
│   ├── database/          # 数据库实现
│   ├── handlers/          # 处理器实现
│   └── utils/             # 工具类实现
│
├── data/                  # 数据目录
│   └── snake.db           # SQLite 数据库（运行时生成）
│
└── build/                 # 构建输出目录
```

## 架构概述

服务器采用分层模块化架构：

1. **数据模型层** (models/): 定义核心数据结构
2. **业务逻辑层** (managers/): 实现游戏逻辑和管理功能
3. **数据持久化层** (database/): 数据库操作和数据存储
4. **HTTP 路由层** (handlers/): 处理 API 请求和响应
5. **工具层** (utils/): 提供通用工具和辅助功能

详细架构说明请参考 [ARCHITECTURE.md](ARCHITECTURE.md)

## 依赖项

- C++17 或更高版本
- CMake 3.15+
- Crow (HTTP 框架，自动下载)
- nlohmann/json (JSON 库，自动下载)
- SQLite3 (嵌入式数据库，需系统安装)

## 构建与运行

### 构建项目

```bash
cd server
mkdir -p build
cd build
cmake ..
make
```

### 运行服务器

```bash
# 使用默认配置
./SnakeServer

# 指定配置文件
./SnakeServer ../config.json
```

## 配置说明

编辑 `config.json` 来修改服务器配置：

```json
{
  "server": {
    "port": 18080,        // 监听端口
    "threads": 4          // 线程数
  },
  "game": {
    "map_width": 50,              // 地图宽度
    "map_height": 50,             // 地图高度
    "round_time_ms": 1000,        // 回合时长（毫秒）
    "initial_snake_length": 3,    // 初始蛇长度
    "invincible_rounds": 5,       // 无敌回合数
    "food_density": 0.05          // 食物密度
  },
  "database": {
    "path": "./data/snake.db",           // 数据库文件路径
    "snapshot_interval": 10,             // 快照间隔（回合）
    "snapshot_retention_hours": 24,      // 快照保留时间
    "backup_enabled": true,              // 是否启用备份
    "backup_interval_hours": 6           // 备份间隔
  },
  "rate_limits": {
    "login_per_hour": 10,     // 登录限制
    "join_per_minute": 5,     // 加入游戏限制
    "move_per_round": 1,      // 移动指令限制
    "map_per_second": 10      // 地图查询限制
  }
}
```

## API 文档

完整的 API 文档请参考项目根目录的 [web.md](../web.md)

### 主要端点

- `GET /api/status` - 获取服务器状态
- `POST /api/game/login` - 玩家登录
- `POST /api/game/join` - 加入游戏
- `GET /api/game/map` - 获取地图状态
- `POST /api/game/move` - 提交移动指令

## 开发状态

当前项目处于开发阶段，已完成：

- ✅ 完整的架构设计文档
- ✅ 所有头文件定义
- ✅ 数据库层实现（DatabaseManager, LeaderboardManager, SnapshotManager）
- ✅ 基础管理器框架代码
- ✅ 路由处理器框架代码
- ✅ 配置文件结构
- ✅ 完整文档体系（ARCHITECTURE.md, DATABASE.md, FILES.md, TODO.md）

待实现功能：

- ⏳ 数据模型的具体实现细节
- ⏳ 游戏逻辑的具体实现细节
- ⏳ API 端点的具体实现细节
- ⏳ 洛谷剪贴板验证
- ⏳ 速率限制器实现
- ⏳ 单元测试
- ⏳ 性能优化

## 开发指南

### 代码风格

- 遵循 Google C++ Style Guide
- 使用 4 空格缩进
- 所有公共接口必须有注释
- 使用命名空间 `snake`

### 线程安全

- 所有共享数据必须使用互斥锁保护
- 在头文件注释中标注线程安全性

### 错误处理

- 使用 RAII 管理资源
- 适当使用异常和返回值
- 记录详细的错误日志

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

MIT License
