#!/usr/bin/env python3
"""
贪吃蛇自动游玩演示
功能：
1. 自动寻找最近的食物
2. 使用BFS寻路算法
3. 自动避让墙壁、自身和其他玩家
4. 死亡后自动重生
"""

import requests
import json
import time
import sys
import random
import argparse
from typing import Dict, List, Optional, Set, Tuple, Deque, Any
from collections import deque

# 服务器配置
BASE_URL = "http://192.168.6.134:18080"
DEMO_UID = "test1001"
DEMO_NAME = "SmartBot"
DEMO_COLOR = "#00D9FF"

# 默认多玩家配置（无参数时使用）
DEFAULT_PLAYERS: List[Dict[str, str]] = [
    {"uid": "test1001", "name": "SmartBot-A", "color": "#00D9FF"},
    {"uid": "test1002", "name": "SmartBot-B", "color": "#FFD500"},
    {"uid": "test1003", "name": "SmartBot-C", "color": "#FF5C93"},
    {"uid": "668972", "name": "SmartBot-D", "color": "#9B59B6"},
]

class SnakeBot:
    def __init__(self, base_url: str, uid: str, name: str, color: str):
        self.base_url = base_url
        self.uid = uid
        self.name = name
        self.color = color
        self.key: Optional[str] = None
        self.token: Optional[str] = None
        self.player_id: Optional[str] = None
        self.last_direction: Optional[str] = None  # 记录上一次的移动方向，防止掉头
        
        # 本地维护的地图状态
        self.local_players: Dict[str, dict] = {}
        self.local_foods: Set[Tuple[int, int]] = set()
        self.current_round: int = 0
        self.map_width: int = 50
        self.map_height: int = 50
        self.round_time: int = 1000
        
        # 统计信息
        self.total_moves: int = 0
        self.foods_eaten: int = 0
        self.deaths: int = 0
        self.max_length: int = 3
        
        self.session = requests.Session()
    
    def log(self, level: str, message: str):
        """打印日志"""
        timestamp = time.strftime("%H:%M:%S")
        colors = {
            "INFO": "\033[36m",     # 青色
            "SUCCESS": "\033[32m",  # 绿色
            "WARNING": "\033[33m",  # 黄色
            "ERROR": "\033[31m",    # 红色
            "RESET": "\033[0m"
        }
        color_code = colors.get(level, colors["RESET"])
        print(f"{color_code}[{timestamp}] [{level}]{colors['RESET']} {message}")
    
    def check_response(self, response: requests.Response, endpoint: str) -> Optional[dict]:
        """检查响应并返回数据"""
        try:
            data = response.json()
            if data.get("code") != 0:
                if data.get("code") == 404:  # 玩家已死亡
                    return None
                self.log("ERROR", f"{endpoint} failed: {data.get('msg')}")
                return None
            return data.get("data", {})
        except Exception as e:
            self.log("ERROR", f"{endpoint} error: {e}")
            return None
    
    def login(self) -> bool:
        """登录获取key"""
        try:
            # 为uid 668972使用特定的paste，其他使用demo_paste
            paste = "eoy1fjmi" if self.uid == "668972" else "demo_paste"
            payload = {"uid": self.uid, "paste": paste}
            response = self.session.post(f"{self.base_url}/api/game/login", json=payload)
            data = self.check_response(response, "Login")
            if data and "key" in data:
                self.key = data["key"]
                self.log("SUCCESS", f"✓ 登录成功")
                return True
        except Exception as e:
            self.log("ERROR", f"登录失败: {e}")
        return False
    
    def join_game(self, update_map_state: bool = True) -> bool:
        """加入游戏"""
        try:
            payload = {"key": self.key, "name": self.name, "color": self.color}
            response = self.session.post(f"{self.base_url}/api/game/join", json=payload)
            data = self.check_response(response, "Join")
            if data and "token" in data and "id" in data:
                self.token = data["token"]
                self.player_id = data["id"]
                # 记录初始方向，防止第一次移动时掉头（规范化为小写）
                initial_dir = data.get("initial_direction", "right").lower()
                self.last_direction = initial_dir
                self.log("SUCCESS", f"✓ 加入游戏成功 (ID: {self.player_id}，初始方向: {initial_dir})")
                
                # 初始化地图状态
                if update_map_state and "map_state" in data:
                    self._update_full_state(data["map_state"])
                
                return True
        except Exception as e:
            self.log("ERROR", f"加入游戏失败: {e}")
        return False
    
    def get_server_status(self) -> bool:
        """获取服务器状态"""
        try:
            response = self.session.get(f"{self.base_url}/api/status")
            data = self.check_response(response, "Status")
            if data:
                self.map_width = data.get("map_size", {}).get("width", 50)
                self.map_height = data.get("map_size", {}).get("height", 50)
                self.round_time = data.get("round_time", 1000)
                self.log("INFO", f"地图大小: {self.map_width}x{self.map_height}, 回合时长: {self.round_time}ms")
                return True
        except Exception as e:
            self.log("ERROR", f"获取服务器状态失败: {e}")
        return False
    
    def get_delta_map(self) -> Optional[dict]:
        """获取增量地图"""
        try:
            response = self.session.get(f"{self.base_url}/api/game/map/delta")
            data = self.check_response(response, "Delta")
            if data and "delta_state" in data:
                return data["delta_state"]
        except Exception as e:
            self.log("ERROR", f"获取增量地图失败: {e}")
        return None
    
    def get_full_map(self) -> Optional[dict]:
        """获取完整地图"""
        try:
            response = self.session.get(f"{self.base_url}/api/game/map")
            data = self.check_response(response, "FullMap")
            if data and "map_state" in data:
                return data["map_state"]
        except Exception as e:
            self.log("ERROR", f"获取完整地图失败: {e}")
        return None
    
    def move(self, direction: str) -> bool:
        """发送移动指令"""
        try:
            direction = direction.lower()  # 规范化为小写
            payload = {"token": self.token, "direction": direction}
            response = self.session.post(f"{self.base_url}/api/game/move", json=payload)
            data = self.check_response(response, f"Move({direction})")
            if data is not None:
                # 移动成功后更新上一次方向
                self.last_direction = direction
                return True
            return False
        except Exception as e:
            return False
    
    def _update_full_state(self, map_state: dict):
        """用完整地图状态更新本地状态"""
        self.current_round = map_state.get("round", 0)
        
        self.local_players.clear()
        for player in map_state.get("players", []):
            self.local_players[player["id"]] = {
                "name": player["name"],
                "color": player["color"],
                "head": (player["head"]["x"], player["head"]["y"]),
                "blocks": [(b["x"], b["y"]) for b in player["blocks"]],
                "length": player["length"],
                "invincible_rounds": player.get("invincible_rounds", 0)
            }
        
        self.local_foods.clear()
        for food in map_state.get("foods", []):
            self.local_foods.add((food["x"], food["y"]))
    
    def _update_delta_state(self, delta: dict):
        """用增量状态更新本地状态"""
        delta_round = delta.get("round", self.current_round)
        if delta_round <= self.current_round:
            return
        if delta_round > self.current_round + 1:
            # 丢帧，回退到完整地图
            full_map = self.get_full_map()
            if full_map:
                self._update_full_state(full_map)
            return

        self.current_round = delta_round
        
        # 移除死亡玩家
        for player_id in delta.get("died_players", []):
            if player_id in self.local_players:
                del self.local_players[player_id]
        
        # 添加新加入的玩家
        for player in delta.get("joined_players", []):
            self.local_players[player["id"]] = {
                "name": player["name"],
                "color": player["color"],
                "head": (player["head"]["x"], player["head"]["y"]),
                "blocks": [(b["x"], b["y"]) for b in player["blocks"]],
                "length": player["length"],
                "invincible_rounds": player.get("invincible_rounds", 0)
            }
        
        # 更新玩家简化信息
        for player_update in delta.get("players", []):
            player_id = player_update["id"]
            if player_id in self.local_players:
                player = self.local_players[player_id]
                new_head = (player_update["head"]["x"], player_update["head"]["y"])
                new_length = player_update["length"]
                
                # 更新blocks
                if player["head"] != new_head:
                    # 头部移动了
                    player["blocks"].insert(0, new_head)
                    while len(player["blocks"]) > new_length:
                        player["blocks"].pop()
                elif len(player["blocks"]) != new_length:
                    # 长度变化（吃到食物）
                    while len(player["blocks"]) < new_length:
                        player["blocks"].append(player["blocks"][-1])
                    while len(player["blocks"]) > new_length:
                        player["blocks"].pop()
                
                player["head"] = new_head
                player["length"] = new_length
                player["invincible_rounds"] = player_update.get("invincible_rounds", 0)
        
        # 移除食物
        for food in delta.get("removed_foods", []):
            self.local_foods.discard((food["x"], food["y"]))
        
        # 添加食物
        for food in delta.get("added_foods", []):
            self.local_foods.add((food["x"], food["y"]))
    
    def get_my_player(self) -> Optional[dict]:
        """获取自己的玩家信息"""
        return self.local_players.get(self.player_id)
    
    def is_opposite_direction(self, dir1: str, dir2: str) -> bool:
        """判断两个方向是否相反"""
        opposites = {
            'up': 'down',
            'down': 'up',
            'left': 'right',
            'right': 'left'
        }
        return opposites.get(dir1) == dir2
    
    def is_valid_pos(self, x: int, y: int) -> bool:
        """检查位置是否在地图内"""
        return 0 <= x < self.map_width and 0 <= y < self.map_height
    
    def is_obstacle(self, x: int, y: int, exclude_player_id: Optional[str] = None) -> bool:
        """检查位置是否是障碍物（其他玩家的身体）"""
        for player_id, player in self.local_players.items():
            if exclude_player_id and player_id == exclude_player_id:
                continue
            # 其他玩家的身体都是障碍物
            if (x, y) in player["blocks"]:
                return True
        return False
    
    def bfs_find_food(self, start: Tuple[int, int]) -> Optional[List[str]]:
        """使用BFS寻找到最近食物的路径，同时遵守掉头限制"""
        if not self.local_foods:
            return None
        
        my_player = self.get_my_player()
        if not my_player:
            return None
        
        # 我的身体块（除了尾部，因为蛇会移动）
        my_blocks = set(my_player["blocks"][:-1]) if len(my_player["blocks"]) > 1 else set()
        
        queue: Deque[Tuple[int, int, List[str]]] = deque([(start[0], start[1], [])])
        visited: Set[Tuple[int, int]] = {start}
        
        directions = {
            'up': (0, -1),
            'down': (0, 1),
            'left': (-1, 0),
            'right': (1, 0)
        }
        
        while queue:
            x, y, path = queue.popleft()
            
            # 到达食物
            if (x, y) in self.local_foods:
                return path
            
            # 探索邻居
            for dir_name, (dx, dy) in directions.items():
                # 避免掉头：如果路径非空且要掉头，跳过
                if path and self.last_direction and self.is_opposite_direction(dir_name, self.last_direction):
                    continue
                
                nx, ny = x + dx, y + dy
                
                # 检查有效性
                if not self.is_valid_pos(nx, ny):
                    continue
                if (nx, ny) in visited:
                    continue
                if (nx, ny) in my_blocks:
                    continue
                if self.is_obstacle(nx, ny, exclude_player_id=self.player_id):
                    continue
                
                visited.add((nx, ny))
                queue.append((nx, ny, path + [dir_name]))
        
        return None
    
    def get_safe_directions(self) -> List[Tuple[str, int]]:
        """获取所有安全的移动方向及其安全度评分"""
        my_player = self.get_my_player()
        if not my_player:
            return []
        
        head_x, head_y = my_player['head']
        my_blocks = set(my_player['blocks'][:-1]) if len(my_player['blocks']) > 1 else set()
        
        directions = {
            'up': (0, -1),
            'down': (0, 1),
            'left': (-1, 0),
            'right': (1, 0)
        }
        
        safe_dirs = []
        
        for dir_name, (dx, dy) in directions.items():
            # 防止掉头：如果这个方向与上一次方向相反，跳过
            if self.last_direction and self.is_opposite_direction(dir_name, self.last_direction):
                continue
            
            next_x, next_y = head_x + dx, head_y + dy
            
            # 基本安全性检查
            if not self.is_valid_pos(next_x, next_y):
                continue
            if (next_x, next_y) in my_blocks:
                continue
            if self.is_obstacle(next_x, next_y, exclude_player_id=self.player_id):
                continue
            
            # 计算安全度（空间评分）
            safety_score = self.calculate_space_safety(next_x, next_y)
            safe_dirs.append((dir_name, safety_score))
        
        # 按安全度降序排序
        safe_dirs.sort(key=lambda x: x[1], reverse=True)
        return safe_dirs
    
    def calculate_space_safety(self, x: int, y: int, depth: int = 3) -> int:
        """计算某个位置的空间安全度（BFS探索可达空间）"""
        my_player = self.get_my_player()
        if not my_player:
            return 0
        
        my_blocks = set(my_player['blocks'][:-1]) if len(my_player['blocks']) > 1 else set()
        
        visited: Set[Tuple[int, int]] = {(x, y)}
        queue: Deque[Tuple[int, int, int]] = deque([(x, y, 0)])
        space_count = 0
        
        directions = [(0, -1), (0, 1), (-1, 0), (1, 0)]
        
        while queue:
            cx, cy, d = queue.popleft()
            space_count += 1
            
            if d >= depth:
                continue
            
            for dx, dy in directions:
                nx, ny = cx + dx, cy + dy
                
                if not self.is_valid_pos(nx, ny):
                    continue
                if (nx, ny) in visited:
                    continue
                if (nx, ny) in my_blocks:
                    continue
                if self.is_obstacle(nx, ny, exclude_player_id=self.player_id):
                    continue
                
                visited.add((nx, ny))
                queue.append((nx, ny, d + 1))
        
        return space_count
    
    def decide_next_move_with_info(self) -> Tuple[Optional[str], Dict[str, Any]]:
        """决策下一步移动方向，并返回调试信息"""
        my_player = self.get_my_player()
        if not my_player:
            return None, {"strategy": "no_player"}
        
        head = my_player['head']
        info: Dict[str, Any] = {
            "round": self.current_round,
            "length": my_player['length'],
            "foods": len(self.local_foods)
        }
        
        # 策略1: 如果有食物，尝试前往最近的食物
        path = self.bfs_find_food(head)
        if path and len(path) > 0:
            target_direction = path[0]
            info["path_length"] = len(path)
            # 验证这个方向是否安全
            safe_dirs = self.get_safe_directions()
            info["safe_dirs"] = safe_dirs
            if any(d[0] == target_direction for d in safe_dirs):
                info["strategy"] = "food"
                return target_direction, info
        
        # 策略2: 没有明确路径到食物，选择最安全的方向
        safe_dirs = self.get_safe_directions()
        info["safe_dirs"] = safe_dirs
        if safe_dirs:
            info["strategy"] = "safe"
            return safe_dirs[0][0], info  # 返回安全度最高的方向
        
        # 策略3: 没有安全方向时，尽量保持上一方向（避免掉头）
        # 或选择相垂直的方向
        info["strategy"] = "fallback"
        all_dirs = {'up', 'down', 'left', 'right'}
        blocked_dirs = set()
        
        my_blocks = set(my_player['blocks'][:-1]) if len(my_player['blocks']) > 1 else set()
        head_x, head_y = head
        
        directions = {
            'up': (0, -1),
            'down': (0, 1),
            'left': (-1, 0),
            'right': (1, 0)
        }
        
        # 找出所有能移动的方向（即使可能死亡）
        moveable_dirs = []
        for dir_name, (dx, dy) in directions.items():
            next_x, next_y = head_x + dx, head_y + dy
            if self.is_valid_pos(next_x, next_y) and (next_x, next_y) not in my_blocks and not self.is_obstacle(next_x, next_y, exclude_player_id=self.player_id):
                # 不掉头优先
                if not (self.last_direction and self.is_opposite_direction(dir_name, self.last_direction)):
                    return dir_name, info
                moveable_dirs.append(dir_name)
        
        # 万不得已才掉头
        if moveable_dirs:
            return moveable_dirs[0], info
        
        # 最后手段：返回上一方向（希望不会真的走到这一步）
        if self.last_direction:
            return self.last_direction, info
        
        return 'right', info
    
    def print_stats(self):
        """打印统计信息"""
        my_player = self.get_my_player()
        if my_player:
            current_length = my_player['length']
            self.max_length = max(self.max_length, current_length)
        else:
            current_length = 0
        
        self.log("INFO", 
                f"回合: {self.current_round} | "
                f"移动: {self.total_moves} | "
                f"长度: {current_length} | "
                f"最大: {self.max_length} | "
                f"食物: {self.foods_eaten} | "
                f"死亡: {self.deaths}")
    
    def run(self):
        """主运行循环"""
        self.log("INFO", "=" * 60)
        self.log("INFO", f"🐍 贪吃蛇自动游玩演示启动")
        self.log("INFO", f"Bot名称: {self.name}")
        self.log("INFO", "=" * 60)
        
        # 初始化
        if not self.get_server_status():
            self.log("ERROR", "无法连接到服务器")
            return False
        
        if not self.login():
            self.log("ERROR", "登录失败")
            return False
        
        if not self.join_game():
            self.log("ERROR", "加入游戏失败")
            return False
        
        # 等待无敌时间结束
        time.sleep(2)
        
        self.log("SUCCESS", "✓ 初始化完成，开始游戏")
        print()
        
        last_length = 3
        moves_since_last_print = 0
        
        try:
            while True:
                # 获取地图状态
                delta = self.get_delta_map()
                if delta:
                    self._update_delta_state(delta)
                else:
                    # 增量获取失败，尝试完整地图
                    full_map = self.get_full_map()
                    if full_map:
                        self._update_full_state(full_map)
                    else:
                        self.log("WARNING", "无法获取地图状态")
                        time.sleep(1)
                        continue
                
                # 检查是否存活
                my_player = self.get_my_player()
                if not my_player:
                    self.log("WARNING", "💀 已死亡，准备重生...")
                    self.deaths += 1
                    self.print_stats()
                    print()
                    
                    # 重新加入
                    if not self.join_game():
                        self.log("ERROR", "重新加入失败")
                        time.sleep(5)
                        continue
                    
                    # 等待无敌时间
                    time.sleep(2)
                    last_length = 3
                    continue
                
                # 检查是否吃到食物
                current_length = my_player['length']
                if current_length > last_length:
                    self.foods_eaten += 1
                    self.log("SUCCESS", f"🍎 吃到食物！长度: {last_length} -> {current_length}")
                last_length = current_length
                
                # 决策下一步
                direction, _ = self.decide_next_move_with_info()
                if not direction:
                    self.log("WARNING", "无法决策移动方向")
                    time.sleep(self.round_time / 1000)
                    continue
                
                # 执行移动
                if self.move(direction):
                    self.total_moves += 1
                    moves_since_last_print += 1
                    
                    # 每10次移动打印一次状态
                    if moves_since_last_print >= 10:
                        self.print_stats()
                        moves_since_last_print = 0
                
                # 等待下一回合
                time.sleep(self.round_time / 1000)
        
        except KeyboardInterrupt:
            self.log("INFO", "\n游戏被用户中断")
            print()
            self.print_stats()
            self.log("INFO", "=" * 60)
            self.log("INFO", "感谢游玩！")
            self.log("INFO", "=" * 60)


class MapClient:
    def __init__(self, base_url: str):
        self.base_url = base_url
        self.session = requests.Session()

    def check_response(self, response: requests.Response, endpoint: str) -> Optional[dict]:
        try:
            data = response.json()
            if data.get("code") != 0:
                return None
            return data.get("data", {})
        except Exception:
            return None

    def get_server_status(self) -> Optional[dict]:
        try:
            response = self.session.get(f"{self.base_url}/api/status")
            return self.check_response(response, "Status")
        except Exception:
            return None

    def get_delta_map(self) -> Optional[dict]:
        try:
            response = self.session.get(f"{self.base_url}/api/game/map/delta")
            data = self.check_response(response, "Delta")
            if data and "delta_state" in data:
                return data["delta_state"]
        except Exception:
            return None
        return None

    def get_full_map(self) -> Optional[dict]:
        try:
            response = self.session.get(f"{self.base_url}/api/game/map")
            data = self.check_response(response, "FullMap")
            if data and "map_state" in data:
                return data["map_state"]
        except Exception:
            return None
        return None


class MultiSnakeController:
    def __init__(self, base_url: str, players: List[Dict[str, str]], verbose: bool = False,
                 stats_interval: int = 10, respawn_delay: float = 2.0):
        self.base_url = base_url
        self.players = players
        self.verbose = verbose
        self.stats_interval = stats_interval
        self.respawn_delay = respawn_delay
        self.map_client = MapClient(base_url)
        self.bots: List[SnakeBot] = []
        self.respawn_after: Dict[str, float] = {}
        self.move_counter = 0

    def setup(self) -> bool:
        status = self.map_client.get_server_status()
        if not status:
            print("无法连接到服务器")
            return False

        map_width = status.get("map_size", {}).get("width", 50)
        map_height = status.get("map_size", {}).get("height", 50)
        round_time = status.get("round_time", 1000)
        print(f"地图大小: {map_width}x{map_height}, 回合时长: {round_time}ms")

        for p in self.players:
            bot = SnakeBot(self.base_url, p["uid"], p["name"], p["color"])
            bot.map_width = map_width
            bot.map_height = map_height
            bot.round_time = round_time
            if not bot.login():
                print(f"登录失败: {p['uid']}")
                return False
            if not bot.join_game(update_map_state=False):
                print(f"加入游戏失败: {p['uid']}")
                return False
            self.bots.append(bot)
        return True

    def run(self):
        if not self.setup():
            return False

        time.sleep(self.respawn_delay)

        last_print = time.time()
        try:
            while True:
                delta = self.map_client.get_delta_map()
                if delta:
                    for bot in self.bots:
                        bot._update_delta_state(delta)
                else:
                    full_map = self.map_client.get_full_map()
                    if full_map:
                        for bot in self.bots:
                            bot._update_full_state(full_map)
                    else:
                        time.sleep(1)
                        continue

                now = time.time()

                for bot in self.bots:
                    my_player = bot.get_my_player()
                    if not my_player:
                        if bot.player_id not in self.respawn_after or now >= self.respawn_after[bot.player_id]:
                            bot.deaths += 1
                            bot.log("WARNING", "💀 已死亡，准备重生...")
                            if bot.join_game(update_map_state=False):
                                self.respawn_after[bot.player_id] = now + self.respawn_delay
                        continue

                    if bot.player_id in self.respawn_after and now < self.respawn_after[bot.player_id]:
                        continue

                    direction, info = bot.decide_next_move_with_info()
                    if not direction:
                        continue

                    if bot.move(direction):
                        bot.total_moves += 1
                        self.move_counter += 1

                        if self.verbose:
                            safe_dirs = info.get("safe_dirs", [])
                            safe_top = safe_dirs[0][1] if safe_dirs else 0
                            bot.log(
                                "INFO",
                                f"Move={direction} | Strat={info.get('strategy')} | "
                                f"Foods={info.get('foods')} | Path={info.get('path_length', 0)} | "
                                f"Safe={len(safe_dirs)} | TopSafe={safe_top}"
                            )

                    if self.move_counter % self.stats_interval == 0 and now - last_print >= 1:
                        for bot in self.bots:
                            bot.print_stats()
                        last_print = now

                time.sleep(self.bots[0].round_time / 1000 if self.bots else 0.1)

        except KeyboardInterrupt:
            print("\n游戏被用户中断")
            for bot in self.bots:
                bot.print_stats()
        return True

def main():
    parser = argparse.ArgumentParser(description="贪吃蛇自动游玩演示")
    parser.add_argument("--base-url", default=BASE_URL)
    parser.add_argument(
        "--player",
        action="append",
        help="玩家配置，格式 uid,name,color，可重复使用",
    )
    parser.add_argument(
        "--players-file",
        help="JSON文件路径，内容为[{uid,name,color}, ...] 或 {players:[...]}"
    )
    parser.add_argument("--verbose", action="store_true", help="输出每步详细日志")
    parser.add_argument("--stats-interval", type=int, default=10, help="统计输出步数间隔")
    parser.add_argument("--respawn-delay", type=float, default=2.0, help="重生无敌等待秒数")

    args = parser.parse_args()

    players: List[Dict[str, str]] = []

    if args.players_file:
        with open(args.players_file, "r", encoding="utf-8") as f:
            data = json.load(f)
            raw_players = data.get("players", data)
            for p in raw_players:
                players.append({
                    "uid": p.get("uid", DEMO_UID),
                    "name": p.get("name", DEMO_NAME),
                    "color": p.get("color", DEMO_COLOR)
                })

    if args.player:
        for item in args.player:
            parts = [p.strip() for p in item.split(",")]
            if len(parts) < 3:
                print(f"玩家配置格式错误: {item}")
                return
            players.append({"uid": parts[0], "name": parts[1], "color": parts[2]})

    if not players:
        players = DEFAULT_PLAYERS.copy()

    if len(players) == 1:
        bot = SnakeBot(args.base_url, players[0]["uid"], players[0]["name"], players[0]["color"])
        bot.run()
    else:
        controller = MultiSnakeController(
            args.base_url,
            players,
            verbose=args.verbose,
            stats_interval=args.stats_interval,
            respawn_delay=args.respawn_delay,
        )
        controller.run()

if __name__ == "__main__":
    main()
