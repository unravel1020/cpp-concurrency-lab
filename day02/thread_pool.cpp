#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    explicit ThreadPool(size_t num_threads)
        : stop_(false)
    {
        for (size_t i = 0; i < num_threads; ++i)
        {
            workers_.emplace_back(
                &ThreadPool::worker,
                this,
                i);
        }
    }

    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stop_ = true;
        }

        cv_.notify_all();

        for (auto& worker : workers_)
        {
            worker.join();
        }
    }

    void submit(std::function<void()> task)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            tasks_.push(std::move(task));
        }

        cv_.notify_one();
    }

private:
    void worker(size_t id)
    {
        while (true)
        {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(mtx_);

                cv_.wait(lock, [this] {
                    return stop_ || !tasks_.empty();
                });

                if (stop_ && tasks_.empty())
                {
                    return;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            std::cout << "worker "
                      << id
                      << " executing task\n";

            task();
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex mtx_;
    std::condition_variable cv_;

    bool stop_;
};

int main()
{
    ThreadPool pool(3);

    for (int i = 0; i < 10; ++i)
    {
        pool.submit([i] {
            std::cout << "task "
                      << i
                      << " start\n";

            std::this_thread::sleep_for(
                std::chrono::milliseconds(500));

            std::cout << "task "
                      << i
                      << " done\n";
        });
    }

    return 0;
}