#include <iostream>
#include <thread>

void worker()
{
    int x = 42;

    std::cout << "worker: x = "
              << x
              << ", address = "
              << &x
              << '\n';
}

int main()
{
    int x = 100;

    std::cout << "main:   x = "
              << x
              << ", address = "
              << &x
              << '\n';

    std::thread t(worker);

    t.join();

    return 0;
}