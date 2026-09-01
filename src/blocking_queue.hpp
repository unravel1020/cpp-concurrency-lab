#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <utility>

// 线程安全的有界阻塞队列。
// 内部使用 condition_variable 实现「队列满时 push 阻塞、队列空时 pop 阻塞」。
template <typename T>
class BlockingQueue {
 public:
  explicit BlockingQueue(size_t capacity) : capacity_(capacity) {}

  BlockingQueue(const BlockingQueue&) = delete;
  BlockingQueue& operator=(const BlockingQueue&) = delete;

  void push(T item) {
    std::unique_lock lock(mu_);
    not_full_.wait(lock, [this] { return queue_.size() < capacity_ || closed_; });
    if (closed_) throw std::runtime_error("push on closed queue");
    queue_.push_back(std::move(item));
    not_empty_.notify_one();
  }

  // 成功取出返回 true；队列已关闭且排空返回 false。
  bool pop(T& out) {
    std::unique_lock lock(mu_);
    not_empty_.wait(lock, [this] { return !queue_.empty() || closed_; });
    if (queue_.empty()) return false;
    out = std::move(queue_.front());
    queue_.pop_front();
    not_full_.notify_one();
    return true;
  }

  void close() {
    std::lock_guard lock(mu_);
    closed_ = true;
    not_empty_.notify_all();
    not_full_.notify_all();
  }

  size_t size() const {
    std::lock_guard lock(mu_);
    return queue_.size();
  }

 private:
  mutable std::mutex mu_;
  std::condition_variable not_empty_;
  std::condition_variable not_full_;
  std::deque<T> queue_;
  size_t capacity_;
  bool closed_ = false;
};
