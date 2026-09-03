#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

std::queue<int> tasks;
std::mutex mtx;
std::condition_variable cv;

void consumer()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [] {
            return !tasks.empty();
        });

        int task = tasks.front();
        tasks.pop();

        lock.unlock();

        std::cout << "consume: " << task << '\n';

        if (task == -1)
        {
            break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(500));
    }
}

void producer()
{
    for (int i = 1; i <= 5; ++i)
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.push(i);
        }

        std::cout << "produce: " << i << '\n';

        cv.notify_one();

        std::this_thread::sleep_for(
            std::chrono::milliseconds(300));
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(-1);
    }

    cv.notify_one();
}

int main()
{
    std::thread consumer_thread(consumer);
    std::thread producer_thread(producer);

    producer_thread.join();
    consumer_thread.join();

    return 0;
}