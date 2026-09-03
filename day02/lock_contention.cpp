#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

using Clock = std::chrono::steady_clock;

long long counter = 0;
std::mutex mtx;

void worker_locked(int iterations)
{
    for (int i = 0; i < iterations; ++i)
    {
        std::lock_guard<std::mutex> lock(mtx);
        ++counter;
    }
}

void worker_local(int iterations)
{
    long long local = 0;

    for (int i = 0; i < iterations; ++i)
    {
        ++local;
    }

    std::lock_guard<std::mutex> lock(mtx);
    counter += local;
}

int main()
{
    constexpr int iterations = 10'000'000;

    // 实验一：每次 ++ 都加锁
    counter = 0;

    auto start = Clock::now();

    std::thread t1(worker_locked, iterations);
    std::thread t2(worker_locked, iterations);

    t1.join();
    t2.join();

    auto end = Clock::now();

    std::cout << "locked every time:\n";
    std::cout << "counter = " << counter << '\n';
    std::cout << "time = "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     end - start)
                     .count()
              << " ms\n\n";

    // 实验二：线程内部先计算，最后只加锁一次
    counter = 0;

    start = Clock::now();

    std::thread t3(worker_local, iterations);
    std::thread t4(worker_local, iterations);

    t3.join();
    t4.join();

    end = Clock::now();

    std::cout << "local first:\n";
    std::cout << "counter = " << counter << '\n';
    std::cout << "time = "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     end - start)
                     .count()
              << " ms\n";

    return 0;
}