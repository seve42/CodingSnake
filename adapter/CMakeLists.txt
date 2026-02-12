cmake_minimum_required(VERSION 3.10)
project(CodingSnake VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# 查找线程库
find_package(Threads REQUIRED)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 示例程序
set(EXAMPLES
    simple_bot
    random_bot
    smart_bot
)

foreach(example ${EXAMPLES})
    add_executable(${example} examples/${example}.cpp)
    target_link_libraries(${example} PRIVATE Threads::Threads)
    
    # 安装到build目录下
    set_target_properties(${example} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
endforeach()

# 打印信息
message(STATUS "CodingSnake Configuration:")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
message(STATUS "Examples to build:")
foreach(example ${EXAMPLES})
    message(STATUS "  - ${example}")
endforeach()
message(STATUS "")
message(STATUS "After building, run:")
message(STATUS "  cd examples && ./simple_bot")
