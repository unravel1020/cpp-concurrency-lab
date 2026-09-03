# 从 `fork()` 理解 Linux 的进程模型

## 1. 为什么今天研究 `fork()`

我学习 AI Systems / LLM Inference 时，不希望只停留在 C++ 和 CUDA API 层面。

像 llama.cpp、SGLang、推理服务这类项目，本质上还是运行在操作系统之上的程序，因此我需要真正理解：

* 什么是进程
* 什么是线程
* 进程之间是什么关系
* 程序启动以后操作系统到底在管理什么
* 为什么一个程序可以产生另一个执行流

所以 Day 01 的 Linux/Systems 部分，我没有按照传统 Linux 教程从命令开始背，而是直接通过实验观察进程。

今天的核心实验是 `fork()`。

---

## 2. 先观察当前进程

首先使用：

```bash
ps -ef | head
ps -T -p $$
cat /proc/$$/status | head -30
```

观察当前 Shell。

我的 Bash 进程当时是：

```text
PID    = 457165
PPID   = 353268
```

使用：

```bash
ps -T -p $$
```

看到：

```text
PID    SPID
457165 457165
```

说明当前 Bash 此时只有一个线程。

然后通过：

```bash
cat /proc/457165/status
```

可以直接看到 Linux 内核提供的进程运行状态。

这让我第一次把之前比较抽象的“进程”对应到了一个真实存在、具有 PID、PPID、内存信息等状态的操作系统对象。

---

## 3. 第一个 `fork()` 实验

代码：

```cpp
#include <iostream>
#include <unistd.h>

int main()
{
    std::cout << "before fork\n";

    pid_t pid = fork();

    std::cout << "after fork\n";

    return 0;
}
```

运行结果：

```text
before fork
after fork
after fork
```

这里最重要的现象是：

```text
before fork
```

只出现一次，而：

```text
after fork
```

出现了两次。

我的理解是：

```text
fork() 前

        一个进程
           │
           │ fork()
           ▼

fork() 后

      parent          child
        │               │
        └──────┬────────┘
               │
        都继续执行后面的代码
```

因此：

```cpp
std::cout << "after fork\n";
```

会被父进程执行一次，也会被子进程执行一次。

这说明 `fork()` 并不是“创建一个进程然后继续执行原来的进程”。

更准确地说：

> **调用 `fork()` 后，原来的进程产生了一个子进程，父子进程都会从 `fork()` 返回之后继续执行。**

---

## 4. 观察 `fork()` 的返回值

然后修改代码：

```cpp
#include <iostream>
#include <unistd.h>

int main()
{
    std::cout << "before fork\n";

    pid_t pid = fork();

    if (pid == 0)
    {
        std::cout << "child:  pid = " << getpid()
                  << '\n';
    }
    else if (pid > 0)
    {
        std::cout << "parent: pid = " << getpid()
                  << ", child pid = " << pid << '\n';
    }
    else
    {
        std::cout << "fork failed\n";
    }

    return 0;
}
```

实际结果：

```text
before fork
parent: pid = 532176, child pid = 532177
child:  pid = 532177
```

这次实验让我理解了 `fork()` 一个非常关键的设计：

### 在父进程中

```cpp
pid_t pid = fork();
```

返回的是：

```text
532177
```

也就是子进程的 PID。

所以：

```cpp
pid > 0
```

表示当前执行的是父进程。

### 在子进程中

同一个：

```cpp
pid_t pid = fork();
```

返回：

```text
0
```

所以：

```cpp
pid == 0
```

表示当前执行的是子进程。

因此可以把它记成：

```text
                    fork()
                      │
              ┌───────┴───────┐
              │               │
           parent           child
              │               │
        return = child PID  return = 0
```

而：

```cpp
getpid()
```

永远返回：

> 当前正在执行这段代码的进程自己的 PID。

---

## 5. 父进程和子进程并不是严格按照顺序运行

我进一步加入：

```cpp
getppid()
```

观察子进程的父进程。

第一次实验得到：

```text
parent: pid = 532662, child pid = 532663
child:  pid = 532663, ppid = 352849
```

这里出现了一个意外结果：

```text
child PID  = 532663
child PPID = 352849
```

按照刚才的理解，本来应该是：

```text
child PPID = 532662
```

为什么不一样？

原因是：

> **父进程和子进程谁先执行是不确定的。**

父进程可能已经打印完信息并退出，而子进程这时才执行：

```cpp
getppid()
```

此时原来的父进程已经不存在了，子进程就会被重新托管，因此看到的 PPID 已经发生变化。

这让我意识到：

**PID/PPID 是运行时状态，而不是程序代码中写死的关系。**

---

## 6. 通过 `sleep()` 验证

为了验证这个推论，我让父进程存活 5 秒：

```cpp
else if (pid > 0)
{
    std::cout << "parent: pid = " << getpid()
              << ", child pid = " << pid << '\n';

    sleep(5);
}
```

同时让子进程等待 2 秒：

```cpp
if (pid == 0)
{
    sleep(2);

    std::cout << "child:  pid = " << getpid()
              << ", ppid = " << getppid() << '\n';
}
```

最终结果：

```text
before fork
parent: pid = 532939, child pid = 532940
child:  pid = 532940, ppid = 532939
```

这次结果符合预期：

```text
parent PID = 532939
child  PID = 532940
child PPID = 532939
```

说明在子进程调用 `getppid()` 时，父进程仍然存在。

---

## 7. 我现在对 `fork()` 的理解

目前我不需要记住大量 Linux API。

我真正需要掌握的是这个模型：

```text
fork() 前

        Process A
           │
           │
         fork()
           │
           ▼

fork() 后

        Process A
        parent
           │
           │
           └────────── Process B
                       child
```

两个进程：

* 有不同的 PID
* 都会从 `fork()` 返回之后继续执行
* 父进程得到子进程 PID
* 子进程得到 0
* 子进程可以通过 `getppid()` 获取父进程 PID
* 父子进程的执行顺序不是由代码简单决定的
* 父进程提前退出后，子进程的 PPID 可能发生变化

---

## 8. 一个容易混淆的地方

`fork()` 并不是简单意义上的：

```text
复制一个程序文件
```

它是操作系统层面的进程创建机制。

从程序员视角，可以理解为：

```text
原来的进程状态
       ↓
    fork()
       ↓
父进程 + 子进程
       ↓
分别继续执行
```

这也是为什么同一行：

```cpp
std::cout << "after fork\n";
```

最终可以执行两次。

至于进程地址空间、内存如何复制，以及为什么实际并不会简单地把整个内存物理复制一份，这涉及 **虚拟内存和 Copy-on-Write**。

这个问题暂时不在今天展开。

---

## 9. 和 AI Systems 有什么关系

`fork()` 本身不是我未来工作的核心 API。

但它让我开始建立一个很重要的底层思维：

> **一个 AI 程序并不是运行在真空里的，它始终处于操作系统的进程、线程、内存和调度模型之中。**

以后分析：

* llama.cpp
* SGLang
* 推理服务
* Worker
* 多进程部署
* 多线程推理
* CPU/GPU 任务协作

时，都需要能够看懂这些操作系统概念。

因此我现在学习 Linux，不是为了成为 Linux 运维人员，而是为了能够真正理解 AI Systems 的运行环境。

---

## 10. 今日总结

今天通过几个实际实验，我确认了：

```text
PID
↓
标识一个进程

PPID
↓
表示当前进程的父进程

fork()
↓
创建子进程

父进程：
fork() → child PID

子进程：
fork() → 0

getpid()
↓
当前进程 PID

getppid()
↓
当前父进程 PID
```

最重要的一句话：

> **`fork()` 之后，父进程和子进程都是独立的执行流，谁先运行是不确定的。**

今天最大的收获不是记住了几个 API，而是第一次通过真实输出观察到了 Linux 进程生命周期。

下一步需要继续理解的是：

**进程和线程到底是什么关系，以及为什么 AI Systems 中大量工作最终都会落到线程、并发和调度上。**
