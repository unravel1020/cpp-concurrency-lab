cmake_minimum_required(VERSION 3.20)
project(cpp_concurrency_lab CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

add_library(concurrency_lab INTERFACE)
target_include_directories(concurrency_lab INTERFACE src)

add_executable(demo examples/demo.cpp)
target_link_libraries(demo PRIVATE concurrency_lab)

include(CTest)
if(BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  FetchContent_MakeAvailable(googletest)

  add_executable(thread_pool_test tests/thread_pool_test.cpp)
  target_link_libraries(thread_pool_test PRIVATE concurrency_lab GTest::gtest_main)
  add_test(NAME thread_pool_test COMMAND thread_pool_test)
endif()

option(BUILD_BENCHMARKS "Build google benchmark targets" ON)
if(BUILD_BENCHMARKS)
  FetchContent_Declare(
    googlebench
    URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
  )
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googlebench)

  add_executable(pool_bench benchmarks/pool_bench.cpp)
  target_link_libraries(pool_bench PRIVATE concurrency_lab benchmark::benchmark)
endif()
