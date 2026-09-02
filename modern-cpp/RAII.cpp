#include <memory>
#include "iostream"

void do_something()
{
  std::cout << "just pretend to do something";
}

void foo()
{
  std::unique_ptr<int> p = std::make_unique<int>(10);
  do_something();
}

int main()
{
  foo();
  return 0;
}
