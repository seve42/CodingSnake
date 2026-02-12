# Snake 类实现说明

## 最新优化（性能与安全）

### 1. 性能优化：使用 std::deque 替代 std::vector

**问题**：`std::vector` 在头部插入元素的时间复杂度是 O(n)，对长蛇性能影响大。

**解决方案**：改用 `std::deque<Point>`
- `push_front()`: O(1) - 头部插入
- `pop_back()`: O(1) - 尾部删除
- 完美适配贪吃蛇的数据结构特点

### 2. API 设计优化：分离方向设置与移动

**问题**：原 `move(Direction dir)` 设计导致方向设置逻辑混乱。

**解决方案**：
- `setDirection(Direction dir)` - 设置下一步移动方向
- `move()` - 使用当前方向移动（无参数）
- 避免了 `move()` 和 `setDirection()` 的逻辑冲突

### 3. 安全性增强：getHead() 异常处理

**问题**：死蛇返回 static Point(0,0) 可能误导调用者。

**解决方案**：
- 死蛇调用 `getHead()` 时抛出 `std::logic_error` 异常
- 强制调用者在调用前检查 `isAlive()`
- 线程安全，无 static 变量

### 4. 健壮性提升：Direction::NONE 处理

**问题**：NONE 状态下仍可能执行移动逻辑。

**解决方案**：
- `move()` 开始时检查 `currentDirection_ == Direction::NONE`
- 未设置方向时直接返回，不执行任何操作
- 双重保护（两处检查）

## 设计特点

### 1. 巧妙的初始化成长机制

传统实现可能在初始化时就创建完整长度的蛇身，这需要特殊处理位置计算。我们采用更优雅的方式：

- **初始化时只占一格**：无论目标长度是多少，蛇初始只有头部一个节点
- **使用 `growthPending_` 计数器**：记录需要成长的次数
- **自然成长**：前 N-1 次移动时不移除尾部，自动成长到目标长度

**优势**：
- 避免了初始化时的位置计算
- 统一了成长逻辑（初始成长和吃食物成长使用相同机制）
- 代码简洁，无需特判

### 2. 严格的参数验证

- 初始长度必须 >= 1，否则抛出 `std::invalid_argument` 异常
- 防止反向移动（不能180度掉头）
- 移动时检查存活状态

### 3. Kill 即销毁

- `kill()` 方法不仅标记死亡，还清空 `blocks_`
- 移除了 `respawn()` 方法，死亡后需要创建新的Snake对象
- 符合"销毁即死亡"的语义

## 核心实现

### 移动逻辑

```cpp
// 使用 setDirection() 设置方向，move() 执行移动
void Snake::move() {
    // 1. 检查存活状态和方向有效性
    if (!alive_ || currentDirection_ == Direction::NONE) {
        return;
    }
    
    // 2. 计算新头部位置
    Point newHead = calculateNewHead(currentDirection_);
    
    // 3. 添加新头部到队列前端 (O(1))
    blocks_.push_front(newHead);
    
    // 4. 根据 growthPending_ 决定是否移除尾部
    if (growthPending_ > 0) {
        growthPending_--;  // 成长：保留尾部
    } else {
        blocks_.pop_back();  // 正常移动：移除尾部 (O(1))
    }
}
```

### 成长逻辑

```cpp
void Snake::grow() {
    growthPending_++;  // 增加待成长次数
}
```

下次移动时自动成长（不移除尾部）。

## 测试覆盖

测试文件：`src/models/test_snake.cpp`

### 测试用例（共15个）

1. **异常处理测试**：验证 initialLength < 1 时抛出异常
2. **初始化测试**：验证长度1和长度3的初始化
3. **成长机制测试**：验证巧妙的成长实现
4. **四方向移动测试**：UP, DOWN, LEFT, RIGHT
5. **防反向移动测试**：验证不能180度掉头
6. **吃食物成长测试**：验证 grow() 方法
7. **自碰撞检测测试**：验证头部不检测，身体检测
8. **身体碰撞检测测试**：验证包括头部的完整检测
9. **无敌状态测试**：验证无敌回合数管理
10. **Kill测试**：验证销毁清空blocks
11. **JSON序列化测试**：验证完整的状态导出
12. **复杂移动序列测试**：验证连续转向
13. **连续吃食物测试**：验证多次grow的累积效果
14. **Direction::NONE测试**：验证未设置方向时不移动
15. **getHead()安全性测试**：验证死蛇调用抛出异常

### 运行测试

```bash
cd build
cmake ..
make test_snake
./test_snake
```

### 测试结果

所有13个测试用例全部通过 ✓

```
========================================
       Snake Class Test Suite
========================================
...
========================================
  ✓ All Tests Passed!
========================================
```

## API 使用示例

### 创建蛇

```cpp
// 在位置(10,10)创建长度为3的蛇
Snake snake(Point(10, 10), 3);
// 此时蛇只占一格：(10,10)
```

### 移动蛇

```cpp
// 第一次移动 - 长度变为2
snake.move(Direction::RIGHT);  // 头部: (11,10), 身体: (10,10)

// 第二次移动 - 长度变为3
snake.move(Direction::RIGHT);  // 头部: (12,10), 身体: (11,10), (10,10)

// 第三次移动 - 长度保持3
snake.move(Direction::RIGHT);  // 头部: (13,10), 身体: (12,10), (11,10)
```

### 吃食物

```cpp
snake.grow();  // 标记待成长
snake.move(Direction::RIGHT);  // 移动时长度+1
```

### 碰撞检测

```cpp
// 检测是否撞到自己的身体（不含头）
bool hitSelf = snake.collidesWithSelf(point);

// 检测是否撞到身体的任意部分（含头）
bool hitBody = snake.collidesWithBody(point);
```

### 死亡处理

```cpp
snake.kill();  // 蛇死亡且被销毁
assert(!snake.isAlive());
assert(snake.getLength() == 0);
```

## 性能特性

- **时间复杂度**：
  - move: O(n)，因为需要 insert 到头部
  - grow: O(1)
  - 碰撞检测: O(n)，n为蛇的长度
  
- **空间复杂度**：O(n)，n为蛇的长度

- **优化空间**：可以使用 `std::deque` 替代 `std::vector` 优化头部插入性能

## 注意事项

1. **初始长度必须 >= 1**，否则抛出异常
2. **死亡后的蛇不能复活**，需要创建新对象
3. **防止反向移动**是自动处理的，调用者无需担心
4. **无敌状态需要手动递减**，通过 `decreaseInvincibleRounds()`
