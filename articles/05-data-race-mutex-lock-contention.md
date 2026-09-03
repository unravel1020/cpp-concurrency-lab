# Data Race、Mutex 与锁竞争：从正确性到性能

## 一、实验目的

这一节主要通过几个 C++ 多线程实验，理解以下问题：

1. 多线程为什么会产生 Data Race。
2. `counter++` 为什么不是一个不可分割的操作。
3. Mutex 如何解决共享数据的并发访问问题。
4. 为什么 `lock_guard` 是 RAII 思想在线程同步中的体现。
5. 为什么“加锁保证正确”并不意味着“程序性能好”。
6. 为什么高性能 C++ 系统需要尽量减少锁竞争和共享状态。

---

# 二、Data Race：为什么两个线程会把加法“算丢”

首先使用两个线程同时对一个全局变量进行累加：

```cpp
#include <iostream>
#include <thread>

int counter = 0;

void worker()
{
    for (int i = 0; i < 1000000; ++i)
    {
        ++counter;
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
```

理论上两个线程各执行 1000000 次：

```text
1000000 + 1000000 = 2000000
```

但这里存在 Data Race。

## 1. `counter++` 并不是一个不可分割的操作

可以近似理解成：

```text
① 读取 counter
② counter + 1
③ 把结果写回 counter
```

例如当前：

```text
counter = 100
```

两个线程可能发生：

```text
线程 A：读取 100
线程 B：读取 100

线程 A：100 + 1 = 101
线程 B：100 + 1 = 101

线程 A：写回 101
线程 B：写回 101
```

最终：

```text
counter = 101
```

而不是：

```text
counter = 102
```

也就是说，两个线程的更新发生了重叠，后一次写入覆盖了前一次更新。

这就是典型的 **Lost Update（更新丢失）**。

## 2. 更严格地说，这是 Undefined Behavior

这里不能简单理解成：

> “多线程运行的时候偶尔会算错。”

因为从 C++ 内存模型来看，多个线程在没有同步机制的情况下，对同一个变量进行冲突访问，会产生 **Data Race**。

Data Race 会导致 **Undefined Behavior（未定义行为）**。

所以即使某次运行恰好得到：

```text
counter = 2000000
```

也不能说明程序是正确的。

---

# 三、Mutex：保证共享数据访问的互斥

为了保护共享变量，引入：

```cpp
std::mutex
```

修改程序：

```cpp
#include <iostream>
#include <thread>
#include <mutex>

int counter = 0;
std::mutex mtx;

void worker()
{
    for (int i = 0; i < 1000000; ++i)
    {
        mtx.lock();
        ++counter;
        mtx.unlock();
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
```

此时：

```cpp
mtx.lock();
++counter;
mtx.unlock();
```

构成一个临界区（Critical Section）。

同一时间只能有一个线程进入这个区域。

原来的情况：

```text
线程 A：读取 100
线程 B：读取 100
线程 A：写 101
线程 B：写 101
```

变成：

```text
线程 A：lock
线程 A：读取 100
线程 A：写 101
线程 A：unlock

线程 B：lock
线程 B：读取 101
线程 B：写 102
线程 B：unlock
```

因此不会发生 Lost Update。

---

# 四、为什么推荐 `lock_guard`

手动：

```cpp
mtx.lock();

do_something();

mtx.unlock();
```

存在一个危险。

如果 `do_something()` 中间发生异常：

```cpp
mtx.lock();

throw std::runtime_error("error");

mtx.unlock();
```

那么 `unlock()` 永远不会执行。

其他线程再次尝试：

```cpp
mtx.lock();
```

就可能一直等待。

因此 C++ 提供了：

```cpp
std::lock_guard<std::mutex>
```

使用方式：

```cpp
std::lock_guard<std::mutex> lock(mtx);
++counter;
```

其核心思想是：

```text
lock_guard 构造
      ↓
   mutex.lock()
      ↓
   进入临界区
      ↓
离开作用域
      ↓
lock_guard 析构
      ↓
  mutex.unlock()
```

这就是之前学习过的 **RAII**。

---

# 五、RAII 在线程同步中的应用

之前通过 `unique_ptr` 学到：

> 资源的获取绑定对象构造，资源的释放绑定对象析构。

现在 Mutex 也可以使用同样的思想。

```text
RAII
 │
 ├── unique_ptr
 │      └── 管理堆内存所有权
 │
 └── lock_guard
        └── 管理 Mutex 锁的生命周期
```

因此：

```cpp
std::lock_guard<std::mutex> lock(mtx);
```

并不仅仅是一个“加锁 API”。

它实际上是利用 C++ 对象生命周期自动管理资源，从而避免人为遗漏 `unlock()`。

---

# 六、实验：Mutex 是否会影响性能？

解决 Data Race 后，程序虽然正确了，但出现了新的问题：

> **锁本身会不会产生性能开销？**

为了验证这一点，设计两个版本。

---

## 版本 A：每次加法都加锁

```cpp
void worker_locked(int iterations)
{
    for (int i = 0; i < iterations; ++i)
    {
        std::lock_guard<std::mutex> lock(mtx);
        ++counter;
    }
}
```

两个线程各执行：

```text
10,000,000 次
```

因此总共进行：

```text
20,000,000 次加法
20,000,000 次 lock
20,000,000 次 unlock
```

---

## 版本 B：线程本地计算，最后只加锁一次

```cpp
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
```

每个线程先操作自己的局部变量：

```text
local
```

最后才访问共享变量：

```text
counter
```

因此两个线程总共只需要进行极少量的同步。

---

# 七、实际实验结果

实验使用：

```text
iterations = 10,000,000
```

连续运行三次。

实际结果：

```text
locked every time:
counter = 20000000
time = 1020 ms

local first:
counter = 20000000
time = 4 ms
```

第二次：

```text
locked every time:
counter = 20000000
time = 1029 ms

local first:
counter = 20000000
time = 4 ms
```

第三次：

```text
locked every time:
counter = 20000000
time = 1124 ms

local first:
counter = 20000000
time = 4 ms
```

结果非常稳定。

第一种方案约：

```text
1020 ~ 1124 ms
```

第二种方案：

```text
4 ms
```

粗略计算，第二种方案大约快了：

```text
250 ~ 280 倍
```

---

# 八、为什么会有这么大的性能差距？

关键并不是 `++` 本身，而是**锁的获取和释放次数以及竞争程度**。

第一种：

```text
线程 1 ─┐
        ├─ lock → ++ → unlock
线程 2 ─┘
        ↓
      lock → ++ → unlock
        ↓
      lock → ++ → unlock
        ↓
       ...
```

两个线程需要围绕同一个共享变量进行大量同步。

总计：

```text
20,000,000 次 lock
20,000,000 次 unlock
```

大量时间并没有花在计算上，而是花在了同步和竞争上。

---

第二种：

```text
线程 1：
local += 10,000,000
        ↓
      lock
        ↓
counter += local
        ↓
      unlock


线程 2：
local += 10,000,000
        ↓
      lock
        ↓
counter += local
        ↓
      unlock
```

线程的大部分工作都在自己的局部变量上完成。

真正需要访问共享数据的地方只有最后一步。

因此：

```text
共享操作：20,000,000 次
```

变成了：

```text
共享更新：2 次
```

这就是巨大的性能差异来源。

---

# 九、重要概念：锁粒度

这个实验让我理解了一个非常重要的性能概念：

> **锁的粒度（Lock Granularity）**

例如：

```text
粗粒度：

lock
 ├── 大量计算
 ├── 数据处理
 ├── 内存操作
 └── 更新
unlock
```

意味着大量工作都被限制在一个临界区中。

而更合理的设计通常是：

```text
线程本地计算
      ↓
线程本地计算
      ↓
线程本地计算
      ↓
非常短的 lock
      ↓
更新共享状态
      ↓
unlock
```

也就是：

> **尽可能缩小临界区，让锁只保护真正需要同步的共享状态。**

---

# 十、从这个实验得到的系统编程思维

这次实验让我意识到：

```text
线程安全
    ↓
加 Mutex
```

只是第一步。

真正做高性能系统时，还需要继续问：

```text
这个锁保护了什么？
        ↓
临界区有多大？
        ↓
有多少线程会竞争？
        ↓
锁被获取多少次？
        ↓
能不能减少共享状态？
        ↓
能不能把计算放到线程本地？
```

因此：

> **“使用 Mutex”解决的是正确性问题，而“如何减少锁竞争”解决的是性能问题。**

这两个问题不能混为一谈。

---

# 十一、与 AI Systems 的联系

在 AI Inference、Runtime、推理引擎等高性能系统中，经常存在：

```text
请求
 ↓
调度
 ↓
Batch
 ↓
模型执行
 ↓
GPU
 ↓
结果
```

如果大量线程频繁竞争同一个全局锁：

```text
Thread 1 ─┐
Thread 2 ─┤
Thread 3 ─┼── Global Mutex
Thread 4 ─┤
Thread 5 ─┘
              ↓
            执行
```

那么即使 GPU 本身非常快，CPU 侧也可能因为同步、等待和锁竞争形成瓶颈。

因此高性能系统经常会尝试：

* 减少共享状态
* 缩小临界区
* 降低锁竞争
* 使用 thread-local 数据
* 批量更新共享状态
* 根据场景考虑原子操作
* 更进一步使用 lock-free 数据结构

这些优化的基础，正是这次实验建立起来的理解。

---

# 十二、这一节的最终理解

这次实验完整串起了：

```text
Process
   ↓
Thread
   ↓
共享地址空间
   ↓
Data Race
   ↓
Lost Update
   ↓
Mutex
   ↓
Critical Section
   ↓
RAII
   ↓
lock_guard
   ↓
Lock Contention
   ↓
降低锁粒度
   ↓
减少共享状态
```

最重要的不是记住几个 API，而是形成两个判断：

### 正确性

> **多个线程访问共享可变状态时，必须建立明确的同步关系，否则可能产生 Data Race 和 Undefined Behavior。**

### 性能

> **同步机制本身也有成本。高性能系统不仅要保证线程安全，还要尽可能减少共享状态、缩小临界区和降低锁竞争。**

这也是从“会写 C++ 多线程代码”进一步走向“理解高性能 C++ 系统”的一个重要节点。
