#include "thread_pool.hpp"

#include <iostream>
#include <string>

int main() {
  ThreadPool pool(4);
  std::cout << "workers: " << pool.worker_count() << "\n";

  std::vector<std::future<int>> futures;
  for (int i = 0; i < 8; ++i) {
    futures.push_back(pool.submit([i] { return i * i; }));
  }
  for (auto& f : futures) {
    std::cout << "result: " << f.get() << "\n";
  }
  return 0;
}
