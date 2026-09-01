// 基准：任务总量固定，扫描线程数与任务粒度。
// 预期观察：线程数超过硬件核数后收益递减，锁竞争与缓存一致性成为瓶颈。
#include "thread_pool.hpp"

#include <benchmark/benchmark.h>

static void BM_SubmitN(benchmark::State& state) {
  const size_t workers = static_cast<size_t>(state.range(0));
  const size_t tasks = static_cast<size_t>(state.range(1));
  for (auto _ : state) {
    ThreadPool pool(workers);
    std::vector<std::future<void>> fs;
    fs.reserve(tasks);
    for (size_t i = 0; i < tasks; ++i) {
      fs.push_back(pool.submit([] {}));
    }
    for (auto& f : fs) f.get();
  }
}
BENCHMARK(BM_SubmitN)->Args({1, 100000})->Args({2, 100000})->Args({4, 100000})
    ->Args({8, 100000})->Args({16, 100000})->Args({32, 100000});

static void BM_BusyTask(benchmark::State& state) {
  const size_t workers = static_cast<size_t>(state.range(0));
  const size_t tasks = static_cast<size_t>(state.range(1));
  for (auto _ : state) {
    ThreadPool pool(workers);
    std::vector<std::future<size_t>> fs;
    fs.reserve(tasks);
    for (size_t i = 0; i < tasks; ++i) {
      fs.push_back(pool.submit([i] {
        size_t acc = i;
        for (int k = 0; k < 1000; ++k) acc += k;  // 模拟中等粒度任务
        return acc;
      }));
    }
    size_t total = 0;
    for (auto& f : fs) total += f.get();
    benchmark::DoNotOptimize(total);
  }
}
BENCHMARK(BM_BusyTask)->Args({1, 2000})->Args({2, 2000})->Args({4, 2000})
    ->Args({8, 2000})->Args({16, 2000})->Args({32, 2000});

BENCHMARK_MAIN();
