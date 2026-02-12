#!/usr/bin/env python3
"""
构建发布版本脚本
自动将带依赖的 CodingSnake.hpp 处理为单文件版本并放置到 release/ 目录
"""

import os
import re
import shutil
from datetime import datetime

def read_file(filepath):
    """读取文件内容"""
    with open(filepath, 'r', encoding='utf-8') as f:
        return f.read()

def write_file(filepath, content):
    """写入文件"""
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

def ensure_dir(dirpath):
    """确保目录存在"""
    os.makedirs(dirpath, exist_ok=True)

def merge_single_file(output_dir='release'):
    """合并生成单文件版本并输出到指定目录"""
    
    print("=" * 60)
    print("CodingSnake 发布版本构建工具")
    print("=" * 60)
    
    # 确保输出目录存在
    ensure_dir(output_dir)
    
    # 读取依赖库
    print("\n[1/5] 读取依赖库...")
    try:
        httplib_content = read_file('third_party/httplib.h')
        print("  ✓ httplib.h")
        json_content = read_file('third_party/json.hpp')
        print("  ✓ json.hpp")
    except FileNotFoundError as e:
        print(f"  ✗ 错误: 找不到依赖文件 {e.filename}")
        return False
    
    # 读取主文件
    print("\n[2/5] 读取 CodingSnake.hpp...")
    try:
        main_content = read_file('CodingSnake.hpp')
        print("  ✓ 主文件读取成功")
    except FileNotFoundError:
        print("  ✗ 错误: 找不到 CodingSnake.hpp")
        return False
    
    # 处理头文件引用
    print("\n[3/5] 处理头文件引用...")
    # 移除对 third_party 的引用
    main_content = re.sub(
        r'#include\s+"third_party/httplib\.h"',
        '// httplib.h content embedded below',
        main_content
    )
    main_content = re.sub(
        r'#include\s+"third_party/json\.hpp"',
        '// json.hpp content embedded below',
        main_content
    )
    print("  ✓ 已移除外部依赖引用")
    
    # 移除依赖库中的 pragma once（避免重复定义）
    httplib_content = re.sub(r'#pragma\s+once', '// #pragma once (removed)', httplib_content)
    json_content = re.sub(r'#pragma\s+once', '// #pragma once (removed)', json_content)
    
    # 构建最终内容
    print("\n[4/5] 合并文件...")
    current_date = datetime.now().strftime("%Y-%m-%d")
    
    final_content = f"""/**
 * CodingSnake.hpp - 贪吃蛇算法竞赛库（单文件版本）
 * 
 * 这是一个完全自包含的单文件库，包含所有依赖
 * 文件大小: ~2MB（包含 httplib 和 nlohmann/json）
 * 
 * 使用方法：
 * 1. 将此文件和你的 main.cpp 放在同一目录
 * 2. 在 main.cpp 中 #include "CodingSnake.hpp"
 * 3. 编译: g++ -std=c++11 main.cpp -o bot -lpthread
 * 
 * 项目地址: https://github.com/yourusername/CodingSnake
 * 
 * @version 1.0.0
 * @date {current_date}
 * @auto-generated 此文件由 build_release.py 自动生成
 */

#ifndef CODING_SNAKE_HPP
#define CODING_SNAKE_HPP

// ============================================================================
// 标准库头文件
// ============================================================================
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <exception>
#include <functional>
#include <sstream>
#include <thread>
#include <chrono>
#include <memory>

// ============================================================================
// 第三方库：cpp-httplib
// GitHub: https://github.com/yhirose/cpp-httplib
// License: MIT
// ============================================================================

{httplib_content}

// ============================================================================
// 第三方库：nlohmann/json
// GitHub: https://github.com/nlohmann/json
// License: MIT
// ============================================================================

{json_content}

// ============================================================================
// CodingSnake 核心实现
// ============================================================================

"""
    
    # 提取主文件中从 "using std::string" 到文件末尾的部分
    match = re.search(r'(using std::string;.*)', main_content, re.DOTALL)
    if match:
        core_content = match.group(1)
        # 移除最后的 #endif
        core_content = re.sub(r'#endif\s*//\s*CODING_SNAKE_HPP\s*$', '', core_content)
        final_content += core_content
    else:
        print("  ✗ 警告: 无法提取核心内容")
        return False
    
    final_content += "\n#endif // CODING_SNAKE_HPP\n"
    print("  ✓ 文件合并完成")
    
    # 写入单文件版本到 release 目录
    print(f"\n[5/5] 写入到 {output_dir}/ 目录...")
    output_file = os.path.join(output_dir, 'CodingSnake.hpp')
    write_file(output_file, final_content)
    
    # 显示文件大小
    size_mb = os.path.getsize(output_file) / (1024 * 1024)
    print(f"  ✓ {output_file}")
    print(f"  ✓ 文件大小: {size_mb:.2f} MB")
    
    # 复制示例文件和文档
    print("\n[额外] 同步其他文件到 release/...")
    files_to_copy = {
        'README_SingleFile.md': 'README.md',  # 单文件版本的 README
        'examples/demo.cpp': 'demo.cpp',       # 示例代码
    }
    
    for src, dst in files_to_copy.items():
        src_path = src
        dst_path = os.path.join(output_dir, dst)
        if os.path.exists(src_path):
            # 如果是目录，复制目录内容
            if os.path.isdir(src_path):
                shutil.copytree(src_path, dst_path, dirs_exist_ok=True)
            else:
                shutil.copy2(src_path, dst_path)
            print(f"  ✓ {src} -> {dst}")
        else:
            print(f"  - 跳过 {src} (不存在)")
    
    # 创建编译脚本
    print("\n[额外] 生成编译脚本...")
    
    # Linux/Mac 编译脚本
    compile_sh = """#!/bin/bash
# 编译 CodingSnake 示例程序

echo "编译中..."
g++ -std=c++11 demo.cpp -o bot -lpthread

if [ $? -eq 0 ]; then
    echo "✓ 编译成功！"
    echo "运行: ./bot"
else
    echo "✗ 编译失败"
    exit 1
fi
"""
    write_file(os.path.join(output_dir, 'compile.sh'), compile_sh)
    os.chmod(os.path.join(output_dir, 'compile.sh'), 0o755)
    print("  ✓ compile.sh")
    
    # Windows 编译脚本
    compile_bat = """@echo off
REM 编译 CodingSnake 示例程序

echo 编译中...
g++ -std=c++11 demo.cpp -o bot.exe -lpthread

if %errorlevel% equ 0 (
    echo 编译成功！
    echo 运行: bot.exe
) else (
    echo 编译失败
    exit /b 1
)
"""
    write_file(os.path.join(output_dir, 'compile.bat'), compile_bat)
    print("  ✓ compile.bat")
    
    print("\n" + "=" * 60)
    print("✓ 构建完成！")
    print("=" * 60)
    print(f"\n发布文件位于: {output_dir}/")
    print(f"  - CodingSnake.hpp     (单文件库, {size_mb:.2f} MB)")
    print(f"  - demo.cpp            (示例代码)")
    print(f"  - compile.sh/bat      (编译脚本)")
    print(f"  - README.md           (使用说明)")
    print(f"\n用户使用方法：")
    print(f"  1. 下载 {output_dir}/ 目录中的所有文件")
    print(f"  2. 运行编译脚本: ./compile.sh  (或 compile.bat)")
    print(f"  3. 运行程序: ./bot")
    
    return True

def clean_release(output_dir='release'):
    """清理 release 目录"""
    if os.path.exists(output_dir):
        print(f"清理 {output_dir}/ 目录...")
        shutil.rmtree(output_dir)
        print("  ✓ 清理完成")
    else:
        print(f"  - {output_dir}/ 目录不存在，跳过清理")

if __name__ == '__main__':
    import sys
    
    # 解析命令行参数
    if len(sys.argv) > 1:
        if sys.argv[1] == 'clean':
            clean_release()
            sys.exit(0)
        elif sys.argv[1] == '--help' or sys.argv[1] == '-h':
            print("用法:")
            print("  python3 build_release.py         # 构建发布版本")
            print("  python3 build_release.py clean   # 清理 release 目录")
            sys.exit(0)
    
    # 执行构建
    try:
        success = merge_single_file()
        sys.exit(0 if success else 1)
    except Exception as e:
        print(f"\n✗ 错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
