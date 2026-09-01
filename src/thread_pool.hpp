#pragma once

#include "blocking_queue.hpp"

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <vector>

// 固定工作线程数的线程池。
// submit() 返回 std::future，用户可通过 future 拿到任务结果或异常。
class ThreadPool {
 public:
  explicit ThreadPool(size_t workers, size_t queue_capacity = 1024)
      : tasks_(queue_capacity), running_(true) {
    for (size_t i = 0; i < workers; ++i) {
      workers_.emplace_back([this] { worker_loop(); });
    }
  }

  ~ThreadPool() {
    tasks_.close();
    for (auto& w : workers_) {
      if (w.joinable()) w.join();
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  template <typename F, typename... Args>
  auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    using R = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<R()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<R> res = task->get_future();
    tasks_.push([task] { (*task)(); });
    return res;
  }

  size_t worker_count() const { return workers_.size(); }

 private:
  void worker_loop() {
    std::function<void()> task;
    while (tasks_.pop(task)) {
      task();
    }
  }

  BlockingQueue<std::function<void()>> tasks_;
  std::vector<std::thread> workers_;
  std::atomic<bool> running_;
};
