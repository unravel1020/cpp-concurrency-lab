#include <iostream>
#include <thread>
#include <mutex>

int counter = 0;
std::mutex mtx;

void worker()
{
    try
    {
        std::lock_guard<std::mutex> lock(mtx);

        ++counter;

        throw std::runtime_error("test exception");
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << '\n';
    }
}

int main()
{
    std::thread t1(worker);
    std::thread t2(worker);

    t1.join();
    t2.join();

    std::cout << "counter = "
              << counter
              << '\n';

    return 0;
}