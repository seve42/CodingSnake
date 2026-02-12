# 数据目录

此目录用于存储 SQLite 数据库文件。

## 文件说明

- `snake.db` - 主数据库文件（自动生成）
- `snake.db-journal` - SQLite 日志文件（自动生成）
- `*.backup` - 数据库备份文件

## 数据库表

### players
存储玩家认证信息（UID、剪贴板、key）

### leaderboard  
存储玩家排行榜数据（分数、击杀、死亡等）

### game_snapshots
存储游戏状态快照（JSON 格式）

## 注意事项

- 数据库文件会在首次启动时自动创建
- 定期备份数据库文件
- 快照文件会根据配置自动清理
