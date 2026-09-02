#include <iostream>
#include <cstddef>

class Buffer
{
public:
    explicit Buffer(std::size_t size)
        : size_(size),
          data_(new char[size])
    {
        std::cout << "construct: "
                  << static_cast<void*>(data_)
                  << '\n';
    }

    ~Buffer()
    {
        std::cout << "destruct: "
                  << static_cast<void*>(data_)
                  << '\n';

        delete[] data_;
    }

    // 禁止拷贝
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // 移动构造
    Buffer(Buffer&& other) noexcept
        : size_(other.size_),
          data_(other.data_)
    {
        // 把资源从 other 手里拿过来
        other.size_ = 0;
        other.data_ = nullptr;

        std::cout << "move\n";
    }

    void print() const
    {
        std::cout << "data = "
                  << static_cast<const void*>(data_)
                  << ", size = "
                  << size_
                  << '\n';
    }

private:
    std::size_t size_;
    char* data_;
};

int main()
{
    Buffer a(1024);

    std::cout << "before move:\n";
    a.print();

    Buffer b = std::move(a);

    std::cout << "after move:\n";

    std::cout << "a: ";
    a.print();

    std::cout << "b: ";
    b.print();

    return 0;
}