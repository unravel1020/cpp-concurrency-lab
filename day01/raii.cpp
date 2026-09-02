// day01/raii.cpp — RAII 生命周期实验
//
// 构建：cmake -S . -B build && cmake --build build
// 运行：./build/raii
//
// 期望输出：
//   constructed
//   inside foo
//   destroyed
//   after foo
//
// 进阶实验（RAII + 栈展开）：
//   把 "inside foo" 改成 throw std::runtime_error("error");
//   并在 main() 里加 try/catch —— 即使发生异常，仍能看到 constructed → destroyed。
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>

class Test
{
public:
    Test()
    {
        std::cout << "constructed\n";
    }

    ~Test()
    {
        std::cout << "destroyed\n";
    }
};

// void foo()
// {
//     auto p = std::make_unique<Test>();

//     std::cout << "inside foo\n";
// }
void foo()
{
    auto p = std::make_unique<Test>();

    std::cout << "before exception" << std::endl;

    throw std::runtime_error("error");
}

void foo_1()
{
    Test* p = new Test();

    std::cout << "before exception" << std::endl;

    throw std::runtime_error("error");
}

int main()
{
    // foo();

    // std::cout << "after foo\n";

    // return 0;
    try
    {
        foo();
    }
    catch(const std::exception& e)
    {
        std::cout << "caught: " << e.what() << std::endl;
    }
    std::cout << "after foo" << std::endl;

    std::cout << std::string(40, '-') << '\n';    
    
    try
    {
        foo_1();
    }
    catch(const std::exception& e)
    {
        std::cout << "caught: " << e.what() << std::endl;
    }
    std::cout << "after foo" << std::endl;

    return 0;
}
