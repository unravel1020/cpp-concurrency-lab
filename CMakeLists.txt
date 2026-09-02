cmake_minimum_required(VERSION 3.20)
project(cpp_concurrency_lab CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()
# MSVC: 源码为 UTF-8（含中文注释），否则预处理器按 GBK 解析会吞掉 include
if(MSVC)
  add_compile_options(/utf-8)
endif()

add_library(concurrency_lab INTERFACE)
target_include_directories(concurrency_lab INTERFACE src)

add_executable(demo examples/demo.cpp)
target_link_libraries(demo PRIVATE concurrency_lab)

include(CTest)
if(BUILD_TESTING)
  # 优先用系统包（apt install libgtest-dev），没有再从网络拉取
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    message(STATUS "GTest not found, fetching googletest from network...")
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    FetchContent_MakeAvailable(googletest)
  endif()

  add_executable(thread_pool_test tests/thread_pool_test.cpp)
  target_link_libraries(thread_pool_test PRIVATE concurrency_lab GTest::gtest_main)
  add_test(NAME thread_pool_test COMMAND thread_pool_test)
endif()

option(BUILD_BENCHMARKS "Build google benchmark targets" ON)
if(BUILD_BENCHMARKS)
  find_package(benchmark QUIET)
  if(NOT benchmark_FOUND)
    message(STATUS "benchmark not found, fetching from network...")
    include(FetchContent)
    FetchContent_Declare(
      googlebench
      URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googlebench)
  endif()

  add_executable(pool_bench benchmarks/pool_bench.cpp)
  target_link_libraries(pool_bench PRIVATE concurrency_lab benchmark::benchmark)
endif()

# ---- 每日实验：dayNN/*.cpp 自动注册为同名可执行文件 ----
# 用法：在 dayNN/ 放一个 .cpp（如 day01/raii.cpp），然后
#   cmake -S . -B build   （首次或新增文件后）
#   cmake --build build   （编译，目标名 = 文件名，如 raii）
#   ./build/raii          （运行）
file(GLOB DAY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/day*/*.cpp")
foreach(day_src ${DAY_SOURCES})
  get_filename_component(day_exe "${day_src}" NAME_WE)
  add_executable(${day_exe} ${day_src})
  target_link_libraries(${day_exe} PRIVATE concurrency_lab)
  message(STATUS "Day target: ${day_exe} <- ${day_src}")
endforeach()
