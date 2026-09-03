# Linux 线程模型：一个进程里的多个线程到底共享什么？

## 1. 为什么学习线程模型

前面通过 `fork()`，我已经实际观察了 Linux 的进程模型：

```text
一个进程
   │
 fork()
   │
 ├── parent
 └── child
```

这一次继续往下研究线程。

我学习 Linux/Systems 的目的并不是成为 Linux 运维人员，而是为了以后能够真正看懂 AI Systems、LLM Inference、ThreadPool、任务调度等代码。

因此我希望理解的不是：

> “线程是轻量级进程。”

而是：

> **一个进程里面出现多个线程以后，它们究竟共享什么，又各自拥有些什么？**

于是这次直接通过实验验证。

---

## 2. 第一个实验：C++ 线程 ID

首先创建一个 worker thread：

```cpp
#include <iostream>
#include <thread>
#include <unistd.h>

void worker()
{
    std::cout << "worker: pid = "
              << getpid()
              << ", tid = "
              << std::this_thread::get_id()
              << '\n';
}

int main()
{
    std::cout << "main:   pid = "
              << getpid()
              << ", tid = "
              << std::this_thread::get_id()
              << '\n';

    std::thread t(worker);

    t.join();

    return 0;
}
```

实际运行结果：

```text
main:   pid = 534754, tid = 135733615744896
worker: pid = 534754, tid = 135733608183488
```

首先观察 PID：

```text
main   PID = 534754
worker PID = 534754
```

两个线程拥有相同的 PID。

但是 C++ 的：

```cpp
std::this_thread::get_id()
```

得到的线程 ID 不同。

这说明：

> **两个线程属于同一个进程，但它们是不同的执行流。**

不过这里的 `std::thread::id` 是 C++ 层面的线程 ID，并不是 Linux 内核意义上的 TID。

所以继续做实验。

---

## 3. 第二个实验：Linux 内核 TID

使用 Linux 的：

```cpp
syscall(SYS_gettid)
```

获取线程的内核 TID。

代码：

```cpp
#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>

long get_tid()
{
    return syscall(SYS_gettid);
}

void worker()
{
    std::cout << "worker: pid = "
              << getpid()
              << ", tid = "
              << get_tid()
              << '\n';
}

int main()
{
    std::cout << "main:   pid = "
              << getpid()
              << ", tid = "
              << get_tid()
              << '\n';

    std::thread t(worker);

    t.join();

    return 0;
}
```

实际结果：

```text
main:   pid = 534879, tid = 534879
worker: pid = 534879, tid = 534880
```

这个结果非常关键。

可以画成：

```text
Process
PID = 534879
│
├── Main Thread
│     TID = 534879
│
└── Worker Thread
      TID = 534880
```

因此：

```text
PID 相同
TID 不同
```

说明：

> **Linux 中多个线程属于同一个进程，但每个线程都有自己的线程 ID。**

主线程的 TID 恰好与进程 PID 相同，因此会看到：

```text
PID = TID
```

而 worker thread：

```text
PID = 534879
TID = 534880
```

这也解释了之前使用：

```bash
ps -T -p <PID>
```

时为什么可以看到相同的 PID 对应多个不同的 SPID。

---

## 4. 第三个实验：线程是否共享数据？

仅仅知道“线程属于同一个进程”还不够。

我真正想验证的是：

> **两个线程到底是不是在访问同一个地址空间？**

于是定义一个全局变量：

```cpp
int value = 0;
```

worker 修改它：

```cpp
void worker()
{
    value = 42;

    std::cout << "worker: value = "
              << value
              << ", address = "
              << &value
              << '\n';
}
```

main 在创建线程前后分别打印：

```cpp
std::cout << "main: value = "
          << value
          << ", address = "
          << &value
          << '\n';
```

实际结果：

```text
main:   value = 0, address = 0x5f4fadf54154
worker: value = 42, address = 0x5f4fadf54154
main:   value = 42, address = 0x5f4fadf54154
```

这个结果非常直接。

worker：

```text
value = 42
```

main 最后也看到了：

```text
value = 42
```

而且三个地方打印出来的：

```text
&value
```

完全相同：

```text
0x5f4fadf54154
```

因此可以确认：

> **同一个进程中的线程共享进程地址空间，因此可以直接访问同一份全局数据。**

不是两个线程各自拥有一个 `value`。

而是：

```text
              Process
                 │
        ┌────────┴────────┐
        │                 │
     Thread 1          Thread 2
        │                 │
        └────────┬────────┘
                 │
            同一个 value
```

---

## 5. 第四个实验：线程是不是共享所有内存？

这里需要避免另一个错误理解：

> “线程共享地址空间，所以线程的所有变量都是共享的。”

并不是。

我把变量改成局部变量：

```cpp
void worker()
{
    int x = 42;

    std::cout << "worker: x = "
              << x
              << ", address = "
              << &x
              << '\n';
}
```

main 中：

```cpp
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
```

实际运行：

```text
main:   x = 100, address = 0x7ffd6cebcc8c
worker: x = 42, address = 0x77b8993fed54
```

两个 `x` 的地址完全不同。

因此：

```text
main thread
    x = 100
    address = 0x7ffd6cebcc8c

worker thread
    x = 42
    address = 0x77b8993fed54
```

说明它们是两个不同的局部变量。

这里体现的是：

> **每个线程都有自己的栈。**

---

## 6. 最终理解：线程到底共享什么？

经过实际实验，现在可以把模型整理成：

```text
                         Process
                      PID = 534879
                           │
             ┌─────────────┴─────────────┐
             │                           │
        Main Thread                Worker Thread
        TID = 534879              TID = 534880
             │                           │
        独立 Stack                  独立 Stack
             │                           │
             └─────────────┬─────────────┘
                           │
                     共享地址空间
                           │
              ┌────────────┼────────────┐
              │            │            │
             Code         Heap       Global Data
```

因此：

### 共享

同一个进程中的线程通常共享：

* 虚拟地址空间
* Code
* Heap
* 全局变量
* 静态变量
* 打开的文件描述符等进程级资源

### 独立

每个线程拥有自己的：

* Thread ID
* 栈
* 寄存器状态
* 程序执行位置
* 线程局部存储等线程级状态

---

## 7. 这和进程有什么区别？

现在可以把刚才的 `fork()` 实验和今天的线程实验放在一起比较。

### 进程

```text
Process A
   │
 fork()
   │
 ├── Process A
 └── Process B
```

进程之间具有独立的地址空间。

### 线程

```text
Process A
   │
 ├── Thread 1
 └── Thread 2
```

线程属于同一个进程，共享地址空间，但拥有独立的执行状态和栈。

所以可以粗略理解为：

> **进程提供资源和地址空间的隔离，线程提供同一进程内部的并发执行。**

这也是为什么多线程通信通常比进程间通信直接得多——线程可以直接访问共享内存。

但这也埋下了一个问题。

---

## 8. 共享数据带来的问题

今天的实验中，worker 可以直接执行：

```cpp
value = 42;
```

而 main 也可以直接读取：

```cpp
value
```

这非常方便。

但是如果两个线程同时修改：

```cpp
value
```

事情就完全不一样了。

例如：

```text
Thread 1 ──────┐
               │
               ├── counter++
               │
Thread 2 ──────┘
```

两个线程同时访问同一块内存，就可能出现：

* 数据竞争
* 更新丢失
* 未定义行为
* 非预期结果

所以：

> **线程共享内存既是优势，也是并发编程最重要的问题来源之一。**

下一步就需要通过实验观察 Data Race。

---

## 9. 和 AI Systems 的联系

这个模型对以后学习 AI Systems 很重要。

例如一个推理服务可能存在：

```text
Inference Process
│
├── Request Thread
├── Scheduler Thread
├── Worker Thread
├── CPU preprocessing
└── GPU submission
```

这些线程可能共享：

```text
模型状态
请求队列
KV Cache 管理信息
任务队列
统计信息
内存池
```

共享内存带来的效率很高，但也意味着必须解决：

```text
并发访问
    ↓
数据竞争
    ↓
同步
    ↓
锁 / 原子操作 / 无锁结构
```

因此今天的线程模型并不是孤立的 Linux 知识。

它实际上是以后理解：

* ThreadPool
* Scheduler
* Concurrent Queue
* LLM Inference Runtime
* llama.cpp
* SGLang

的重要基础。

---

## 10. 我的最终理解

今天通过实际代码，我验证了：

```text
一个进程
    │
    ├── Thread 1
    └── Thread 2
```

线程：

```text
共享：
    地址空间
    Heap
    Global Data
    Code

独立：
    Stack
    Registers
    TID
    执行状态
```

最重要的一句话：

> **线程共享进程的地址空间，但每个线程拥有自己的执行状态和栈。**

这也解释了一个看似矛盾的现象：

```text
为什么两个线程可以访问同一个全局变量？
```

因为：

```text
共享地址空间
```

同时：

```text
为什么两个线程的局部变量地址不同？
```

因为：

```text
每个线程拥有自己的栈
```

而下一步真正需要解决的问题就是：

> **如果两个线程同时修改同一个共享变量，会发生什么？**
