你好，这里是CodingSnake文档

你不需要懂 `class`、`http` 等复杂概念，只需要用你最熟悉的 `C/C++` 语法，即可开始。

`CodingSnake.hpp` 是我们的库，它把所有复杂的网络通信、数据处理都封装好了，你不需要关注任何东西。你只需要`#include "CodingSnake.hpp"`，然后专注于实现一个函数：`decide`。

---

##  第零步：获取库和登录token

要开始编程，你首先需要 `CodingSnake.hpp` 这个文件。

[点击这里下载 `CodingSnake.hpp`](https://raw.githubusercontent.com/seve42/CodingSnake/main/adapter/release/CodingSnake.hpp)

想要加入游戏，你还需要一个账号。我们目前使用luogu帐号来辅助验证。用你的账号创建一个云剪贴板，内容仅包含`CodingSnake2026`，然后将链接`https://www.luogu.com.cn/paste/xxxxxx`中的`xxxxxx`部分复制下来，这就是你的paste。

---

##  第一步：快速开始（3个步骤）

### 1. 准备文件
将下载好的 `CodingSnake.hpp` 和你的代码文件（例如 `main.cpp`）放在同一个文件夹下。

目录结构应该像这样：
```
my_bot/
├── CodingSnake.hpp
└── main.cpp
```

你可以用任意IDE或文本编辑器来编写 `main.cpp`，例如dev-cpp或是vscode。只要确保它能找到 `CodingSnake.hpp` 就行了。

### 2. 编写你的第一个“机器人”
创建 `main.cpp`，内容如下。这是一个最简单的机器人，它只会一直向右走。


```cpp
#include "CodingSnake.hpp" // 我们的库
#include <bits/stdc++.h> 
// 这是你唯一需要编写的函数
// state 包含了当前游戏的所有信息
string decide(const GameState& state) {
    // 在这里写你的逻辑
    return "right"; // 决策：一直向右
}

int main() {
    // 1. 连接到服务器
    CodingSnake game("http://csapi.seveoi.icu:18080"); //我们的服务器地址

    // 2. 登录（把 "uid" 和 "paste" 换成你的）
    game.login("your_uid", "your_paste");

    // 3. 加入游戏，给你的蛇起个名字
    game.join("MyBot");

    // 4. 运行你的决策函数！
    game.run(decide);

    return 0;
}
```

### 3. 编译并运行

现在，你的第一个CodingSnake应该已经成功运行了！

### 4. `decide` 超过回合时间会怎样？

这是一个很重要的点：**SDK 不会强制中断你的 `decide` 函数**。

- 如果 `decide` 运行太久（超过一个回合），这段时间内不会发送新移动指令。
- 服务器会让你的蛇继续按“上一方向”前进，因此你会错过一个或多个回合的操作机会。
- 等 `decide` 终于返回后，SDK 才会提交这次方向；通常已经是更晚的回合了。

结论：请尽量让 `decide` 在一个回合时间内完成（越快越好）。

---

##  第二步：一个简单的随机游走(bushi)机器人

只会向右走太蠢了，我们来让它随机游走。

只需要修改 `decide` 函数，其他都不用变。

```cpp
#include "CodingSnake.hpp"
#include <cstdlib> // 用于 rand()
#include <ctime>   // 用于 time()

string decide(const GameState& state) {
    // 准备一个方向数组
    string directions[] = {"up", "down", "left", "right"};
    
    // 随机选择一个方向
    int randomIndex = rand() % 4;
    string randomDirection = directions[randomIndex];

    return randomDirection;
}

int main() {
    srand(time(0));

    CodingSnake game("http://csapi.seveoi.icu:18080");
    game.login("your_uid", "your_paste");
    game.join("RandomBot");
    game.run(decide);
    
    return 0;
}
```
这个机器人会随机选择方向，比之前的好多了，但它还是可能会撞墙或撞自己。你能优化它吗？

---

##  第三步：一个更聪明的机器人

这个示例展示了如何寻找最近的食物，并朝它移动。这是一个不错的入门策略。

**注意，如果你看不懂库中的一些信息，可以跳过这一部分，下面有详细的讲解**

```cpp
#include "CodingSnake.hpp"
#include <vector>
#include <string>

// 决策函数：每回合自动调用
string decide(const GameState& state) {
    Snake me = state.getMySnake();          // 获取关于“我”的信息
    vector<Point> foods = state.getFoods(); // 获取所有食物的位置

    // 如果没有食物，就随便走走
    if (foods.empty()) {
        return "right";
    }

    // 1. 找到最近的食物
    Point target = foods[0];
    int min_dist = me.head.distance(target);

    for (size_t i = 1; i < foods.size(); ++i) {
        int dist = me.head.distance(foods[i]);
        if (dist < min_dist) {
            min_dist = dist;
            target = foods[i];
        }
    }

    // 2. 尝试朝食物移动
    // 检查哪个方向离食物更近，并且是安全的
    string best_dir = "right"; // 默认方向
    int current_dist = me.head.distance(target);

    // 向上
    Point next_up = {me.head.x, me.head.y - 1};
    if (state.isValidPos(next_up.x, next_up.y) && !state.hasObstacle(next_up.x, next_up.y)) {
        if (next_up.distance(target) < current_dist) {
            return "up";
        }
    }
    // 向下
    Point next_down = {me.head.x, me.head.y + 1};
    if (state.isValidPos(next_down.x, next_down.y) && !state.hasObstacle(next_down.x, next_down.y)) {
        if (next_down.distance(target) < current_dist) {
            return "down";
        }
    }
    // 向左
    Point next_left = {me.head.x - 1, me.head.y};
    if (state.isValidPos(next_left.x, next_left.y) && !state.hasObstacle(next_left.x, next_left.y)) {
        if (next_left.distance(target) < current_dist) {
            return "left";
        }
    }
    // 向右
    Point next_right = {me.head.x + 1, me.head.y};
    if (state.isValidPos(next_right.x, next_right.y) && !state.hasObstacle(next_right.x, next_right.y)) {
        if (next_right.distance(target) < current_dist) {
            return "right";
        }
    }

    // 如果没有更好的方向，就随便找个安全的方向
    string safe_dirs[] = {"up", "down", "left", "right"};
    for(int i=0; i<4; ++i) {
        Point p = me.head;
        if (safe_dirs[i] == "up") p.y--;
        if (safe_dirs[i] == "down") p.y++;
        if (safe_dirs[i] == "left") p.x--;
        if (safe_dirs[i] == "right") p.x++;
        if (state.isValidPos(p.x, p.y) && !state.hasObstacle(p.x, p.y)) {
            return safe_dirs[i];
        }
    }

    return "right"; // 如果无路可走，就自杀吧
}

int main() {
    try {
        CodingSnake game("http://csapi.seveoi.icu:18080");
        game.login("uid", "paste");
        game.join("GreedyBot", "#00D9FF"); // 可以指定颜色
        game.setVerbose(true);            // 开启详细日志，方便调试
        
        game.run(decide);
    } catch (const SnakeException& e) {
        // 捕获并打印游戏过程中可能出现的错误
        cerr << "错误: " << e.what() << endl;
        return 1;
    }
    return 0;
}
```

---

## 📚 API 速查手册

你的 `decide` 函数会收到一个 `GameState` 结构体，你可以通过它获取游戏世界的一切信息。

### `GameState` - 游戏状态

| 成员函数名 | 返回类型 | 说明 |
| --- | --- | --- |
| `getMySnake()` | `Snake` | 获取代表你自己的那条蛇 |
| `getAllPlayers()` | `vector<Snake>` | 获取场上所有的蛇（包括你） |
| `getOtherPlayers()` | `vector<Snake>` | 获取场上其他的蛇（不包括你） |
| `getFoods()` | `vector<Point>` | 获取所有食物的位置 |
| `getMapWidth()` | `int` | 获取地图宽度 |
| `getMapHeight()` | `int` | 获取地图高度 |
| `getCurrentRound()` | `int` | 获取当前是第几回合 |
| `isValidPos(x, y)` | `bool` | 检查坐标 `(x, y)` 是否在地图内 |
| `hasObstacle(x, y)` | `bool` | 检查坐标 `(x, y)` 是否有障碍物（任何蛇的身体） |

### `Snake` - 蛇
这个结构体代表一条蛇（可能是你，也可能是别人）。

| 成员变量 | 类型 | 说明 |
| --- | --- | --- |
| `id` | `string` | 蛇的唯一ID |
| `name` | `string` | 蛇的名字 |
| `head` | `Point` | 蛇头所在的坐标 |
| `blocks` | `vector<Point>` | 蛇身体所有格子的坐标列表 |
| `length` | `int` | 蛇的长度 |
| `invincible_rounds` | `int` | 剩余的无敌回合数（刚复活时 > 0） |

| 成员函数名 | 返回类型 | 说明 |
| --- | --- | --- |
| `contains(p)` | `bool` | 检查点 `p` 是否在这条蛇的身体上 |
| `isInvincible()` | `bool` | 检查这条蛇是否处于无敌状态 |

### `Point` - 坐标点
一个简单的坐标，表示地图上的一个格子。

| 成员变量 | 类型 | 说明 |
| --- | --- | --- |
| `x` | `int` | 横坐标 |
| `y` | `int` | 纵坐标 |

| 成员函数名 | 返回类型 | 说明 |
| --- | --- | --- |
| `distance(other)` | `int` | 计算和另一个点 `other` 的曼哈顿距离（`abs(x1-x2) + abs(y1-y2)`） |

---

##  常见问题

### 链接器报错

**问题**：编译时遇到链接器报错（通常会看到 `undefined reference to`、`unresolved external symbol` 等信息）。

#### Dev-C++ 


1. 在菜单栏点击 **工具 (Tools)** → **编译选项 (Compiler Options)**（或者如果是项目，选 **项目 (Project)** → **项目属性 (Project Options)**）
2. 找到 **编译器时加入以下命令 (Add the following commands when calling the compiler)** 或者 **链接器 (Linker)** 选项卡
3. 在框中输入以下内容（确保勾选该选项）：
   ```
   -lws2_32
   ```
   （注意：是小写的 L，不是数字 1，即 link ws2_32）
4. 点击确定，重新编译即可

#### VS Code (g++ / MinGW)

你需要修改 `.vscode` 文件夹下的 `tasks.json` 文件。在 `args` 列表中添加 `"-lws2_32"`：

```json
"args": [
    "-fdiagnostics-color=always",
    "-g",
    "${file}",
    "-o",
    "${fileDirname}\\${fileBasenameNoExtension}.exe",
    "-lws2_32"  // 添加这一行，最好放在源文件参数之后
],
```

#### 命令行 (g++)

在你的编译命令最后加上 `-lws2_32`：

```bash
g++ C:/Users/71415/Desktop/untitled1.cpp -o untitled1.exe -lws2_32
```

#### CMake

在你的 `CMakeLists.txt` 文件中，找到 `add_executable` 之后，添加：

```cmake
target_link_libraries(你的项目名称 ws2_32)
```

---

现在，你已经掌握了所有必要的知识。发挥你的智慧，编写出最强的贪吃蛇 AI 吧！
