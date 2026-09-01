#include "thread_pool.hpp"

#include <gtest/gtest.h>

TEST(ThreadPoolTest, SumsTasks) {
  ThreadPool pool(4);
  constexpr int kN = 100;
  std::vector<std::future<int>> futures;
  for (int i = 0; i < kN; ++i) {
    futures.push_back(pool.submit([i] { return i; }));
  }
  int sum = 0;
  for (auto& f : futures) sum += f.get();
  EXPECT_EQ(sum, kN * (kN - 1) / 2);
}

TEST(ThreadPoolTest, PropagatesException) {
  ThreadPool pool(2);
  auto f = pool.submit([]() -> int { throw std::runtime_error("boom"); });
  EXPECT_THROW(f.get(), std::runtime_error);
}

TEST(ThreadPoolTest, SingleWorkerStillWorks) {
  ThreadPool pool(1);
  auto f = pool.submit([] { return 42; });
  EXPECT_EQ(f.get(), 42);
}
