# RAII：从一个 unique_ptr 实验理解 C++ 资源生命周期

## 1. 为什么开始学习 RAII

以前对 C++ 的 RAII（Resource Acquisition Is Initialization）有过一些了解，但理解比较停留在：

> `unique_ptr` 可以自动释放内存。

这次重新学习 C++ 时，我希望不只是记住这个结论，而是通过实际代码观察对象的构造、析构以及异常情况下的行为，从而真正理解 RAII 的工作方式。

---

## 2. 最简单的 RAII 实验

首先创建一个 `Test` 类，在构造函数和析构函数中分别输出信息：

```cpp
#include <iostream>
#include <memory>

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

void foo()
{
    auto p = std::make_unique<Test>();

    std::cout << "inside foo\n";
}

int main()
{
    foo();

    std::cout << "after foo\n";

    return 0;
}
```

运行结果：

```text
constructed
inside foo
destroyed
after foo
```

从输出可以观察到：

1. 进入 `foo()` 后创建 `Test` 对象。
2. `inside foo` 正常输出。
3. `foo()` 结束时，`p` 离开作用域。
4. `unique_ptr` 被析构，同时释放它所管理的 `Test` 对象。
5. `Test::~Test()` 被调用，因此输出 `destroyed`。
6. 最后才回到 `main()` 输出 `after foo`。

也就是说，对象的生命周期与作用域绑定在了一起。

可以简单理解为：

```text
进入 foo()
    ↓
创建 Test
    ↓
unique_ptr 获得 Test 的所有权
    ↓
执行 foo()
    ↓
离开 foo()
    ↓
unique_ptr 析构
    ↓
Test 析构
    ↓
资源释放
```

---

## 3. RAII 在异常情况下仍然有效

接下来测试异常。

将 `foo()` 修改为：

```cpp
void foo()
{
    auto p = std::make_unique<Test>();

    std::cout << "before exception\n";

    throw std::runtime_error("error");
}
```

并在 `main()` 中使用 `try/catch`：

```cpp
int main()
{
    try
    {
        foo();
    }
    catch (const std::exception& e)
    {
        std::cout << "caught: " << e.what() << '\n';
    }

    std::cout << "after foo\n";

    return 0;
}
```

实际运行结果：

```text
constructed
before exception
destroyed
caught: error
after foo
```

这里最值得注意的是：

```text
before exception
destroyed
caught: error
```

`destroyed` 出现在 `catch` 之前。

这说明 `throw` 并不是简单地直接跳到 `catch`。

发生异常后，C++ 会进行 **Stack Unwinding（栈展开）**，在离开当前作用域的过程中，已经构造完成的局部对象仍然会正常析构。

因此实际过程是：

```text
throw
 ↓
开始异常处理
 ↓
Stack Unwinding
 ↓
离开 foo() 作用域
 ↓
p 析构
 ↓
Test 析构
 ↓
继续寻找 catch
 ↓
catch 捕获异常
```

这也是 RAII 非常重要的一个价值：

> 即使函数不是正常返回，而是因为异常离开作用域，局部对象依然会按照生命周期规则进行析构。

---

## 4. 如果不用 unique_ptr 会怎么样？

为了进一步验证 RAII 的意义，将：

```cpp
auto p = std::make_unique<Test>();
```

改成：

```cpp
Test* p = new Test();
```

也就是说改成裸指针。

在同样发生异常的情况下：

```cpp
void foo()
{
    Test* p = new Test();

    std::cout << "before exception\n";

    throw std::runtime_error("error");
}
```

这时 `p` 本身只是一个保存地址的指针变量。

当 `foo()` 因为异常离开时：

```text
p 这个局部变量消失
        ↓
Test 对象仍然存在于堆上
        ↓
没有执行 delete
        ↓
内存泄漏
```

这与 `unique_ptr` 的行为形成了明显对比。

### unique_ptr

```text
unique_ptr
    ↓
拥有 Test
    ↓
离开作用域
    ↓
自动析构
    ↓
Test 被释放
```

### 裸指针

```text
Test*
    ↓
只保存地址
    ↓
离开作用域
    ↓
指针变量消失
    ↓
Test 仍然存在
    ↓
没有 delete
    ↓
内存泄漏
```

因此可以发现：

> 裸指针本身并不代表资源所有权。

---

## 5. RAII 的核心理解

需要注意的是，对于RAII 的理解不应该只是：

> `unique_ptr` 可以自动释放内存。

而是：

> **RAII 的核心是把资源的生命周期绑定到对象的生命周期。**

对象创建时获得资源，对象析构时释放资源。

因此 RAII 并不只适用于堆内存。

它同样可以用于：

* 文件
* Socket
* Mutex / Lock
* 线程相关资源
* 数据库连接
* CUDA 资源
* GPU Buffer
* 其他需要显式申请和释放的资源

例如：

```cpp
{
    std::lock_guard<std::mutex> lock(mutex);

    // 临界区
}
```

`lock_guard` 就是典型的 RAII。

创建 `lock_guard` 时获得锁，离开作用域时自动释放锁。

---

## 6. 一个重要的思维转变

学习 RAII 后，我觉得 C++ 中一个非常重要的思维方式是：

> 不要只关注“这个资源什么时候申请”，还要关注“谁拥有它，以及谁负责释放它”。

例如：

```cpp
Test* p = new Test();
```

看到这里不能只想到：

```text
创建了一个 Test
```

还应该立即想到：

```text
谁负责 delete？
```

而：

```cpp
auto p = std::make_unique<Test>();
```

则已经明确表达了：

```text
p 拥有 Test
 ↓
p 生命周期结束
 ↓
Test 自动释放
```

这实际上是通过 C++ 类型系统表达 ownership。

---

## 7. 本次实验结论

本次实验验证了几个结论：

1. `unique_ptr` 可以通过析构函数自动释放所管理的资源。
2. RAII 的核心不是“自动 delete”，而是**资源生命周期与对象生命周期绑定**。
3. 函数正常退出时，局部对象会析构。
4. 函数因为异常退出时，Stack Unwinding 同样会触发已经构造完成的局部对象析构。
5. 裸指针本身不具有资源所有权语义。
6. RAII 不仅可以管理内存，也可以管理锁、文件、Socket、GPU 资源等各种资源。

记住一句话：

> **谁拥有资源，谁负责资源的生命周期；RAII 则把这种责任交给对象的生命周期管理。**
