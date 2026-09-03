# ThreadPool：从线程同步到任务调度

## 1. 为什么需要 ThreadPool

前面的实验已经看到，创建线程本身存在成本。

如果每来一个任务就：

```cpp
std::thread t(task);
t.join();
```

那么大量短任务会不断创建和销毁线程。

ThreadPool 的思路是：

```text
提前创建一批 Worker Thread
          ↓
       Task Queue
          ↓
Worker 从队列获取任务
          ↓
       执行任务
          ↓
继续等待下一个任务
```

线程不再随着任务创建和销毁，而是被重复利用。

---

## 2. ThreadPool 的核心组成

这次实现主要由五部分组成：

```text
ThreadPool
├── workers_          Worker 线程
├── tasks_            任务队列
├── mtx_              保护任务队列
├── cv_               通知 Worker
└── stop_             控制线程池关闭
```

对应关系：

| 成员         | 作用              |
| ---------- | --------------- |
| `workers_` | 保存 Worker 线程    |
| `tasks_`   | 保存等待执行的任务       |
| `mtx_`     | 保护共享任务队列        |
| `cv_`      | 没任务时让 Worker 等待 |
| `stop_`    | 控制线程池退出         |

---

## 3. submit：提交任务

核心代码：

```cpp
void submit(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        tasks_.push(std::move(task));
    }

    cv_.notify_one();
}
```

执行过程：

```text
submit(task)
    ↓
加锁
    ↓
task 放入 queue
    ↓
解锁
    ↓
notify_one()
    ↓
唤醒一个 Worker
```

这里有一个重要设计：

**任务执行不在锁里面。**

也就是说：

```cpp
lock
    tasks_.push(...)
unlock

task();
```

而不是：

```cpp
lock
    tasks_.push(...)
    task()
unlock
```

因为任务可能执行很长时间。

如果把任务执行放进锁里面，就会导致其他 Worker 无法访问任务队列，线程池实际上会退化成接近串行执行。

这与之前的“缩小临界区”实验是同一个思想。

---

## 4. Worker 的工作循环

Worker 的核心逻辑：

```cpp
while (true)
{
    std::function<void()> task;

    {
        std::unique_lock<std::mutex> lock(mtx_);

        cv_.wait(lock, [this] {
            return stop_ || !tasks_.empty();
        });

        if (stop_ && tasks_.empty())
        {
            return;
        }

        task = std::move(tasks_.front());
        tasks_.pop();
    }

    task();
}
```

可以拆成：

```text
Worker
  ↓
检查任务队列
  ↓
没有任务？
  ↓
wait()
  ↓
进入睡眠
  ↓
被 notify 唤醒
  ↓
重新检查条件
  ↓
取得任务
  ↓
退出锁
  ↓
执行任务
  ↓
回到循环
```

因此 Worker 并不是：

```text
创建线程
 ↓
执行一个任务
 ↓
线程结束
```

而是：

```text
创建线程
 ↓
等待任务
 ↓
执行任务
 ↓
等待任务
 ↓
执行任务
 ↓
等待任务
 ↓
...
```

这就是“线程复用”。

---

## 5. 为什么使用 condition_variable

如果没有 `condition_variable`，Worker 可能需要不断轮询：

```cpp
while (tasks_.empty())
{
    // 不断检查
}
```

这会造成 CPU 空转。

使用：

```cpp
cv_.wait(lock, predicate);
```

之后：

```text
没有任务
 ↓
释放 mutex
 ↓
线程睡眠
 ↓
任务到达
 ↓
notify_one()
 ↓
线程被唤醒
 ↓
重新获取 mutex
 ↓
检查任务队列
```

因此：

**mutex 负责保护共享状态，condition_variable 负责等待和唤醒。**

二者职责不同。

---

## 6. 为什么必须重新检查条件

之前的 Producer–Consumer 实验已经验证：

```cpp
cv.wait(lock);
```

是不安全的。

正确写法：

```cpp
cv.wait(lock, [this] {
    return stop_ || !tasks_.empty();
});
```

这里的 predicate 非常重要。

因为：

> 线程被唤醒 ≠ 条件一定满足。

Worker 被唤醒以后，仍然必须确认：

```cpp
stop_ || !tasks_.empty()
```

是否成立。

这也是为什么条件变量通常和 `while` / predicate 一起使用。

---

## 7. 为什么取出任务之后立即解锁

代码：

```cpp
task = std::move(tasks_.front());
tasks_.pop();
```

之后离开作用域：

```cpp
}
```

`unique_lock` 自动释放 mutex。

然后才：

```cpp
task();
```

这样任务执行期间不占用任务队列的锁。

例如：

```text
Worker 0
    ↓
取 task 0
    ↓
释放锁
    ↓
执行 task 0


Worker 1
    ↓
可以同时获取 task 1
    ↓
释放锁
    ↓
执行 task 1
```

因此多个 Worker 才能真正并发执行。

---

## 8. ThreadPool 的关闭过程

析构函数：

```cpp
~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        stop_ = true;
    }

    cv_.notify_all();

    for (auto& worker : workers_)
    {
        worker.join();
    }
}
```

关闭流程：

```text
ThreadPool 析构
      ↓
stop_ = true
      ↓
notify_all()
      ↓
唤醒所有 Worker
      ↓
Worker 检查 stop_
      ↓
如果还有任务 → 继续执行
      ↓
任务全部完成
      ↓
stop_ && tasks_.empty()
      ↓
Worker return
      ↓
join()
      ↓
ThreadPool 析构完成
```

这里再次体现了 RAII。

ThreadPool 对线程拥有生命周期管理责任，因此在析构时必须保证线程正确结束。

---

## 9. 为什么不是直接 stop 就退出

当前实现：

```cpp
if (stop_ && tasks_.empty())
{
    return;
}
```

意味着：

```text
stop_ = true
        +
tasks_ 为空
        ↓
退出
```

如果：

```text
stop_ = true
        +
tasks_ 不为空
        ↓
继续处理任务
```

因此析构 ThreadPool 时，已经提交到队列中的任务不会直接丢失。

如果写成：

```cpp
if (stop_)
{
    return;
}
```

那么一旦线程池开始关闭，队列中剩余任务可能直接被丢弃。

---

# 10. Worker 数量实验

这次对 ThreadPool 分别进行了：

```text
1 worker
2 workers
3 workers
4 workers
8 workers
16 workers
```

每个任务固定执行：

```cpp
std::this_thread::sleep_for(
    std::chrono::milliseconds(500));
```

总共：

```text
10 个任务
```

因此理论执行时间大致为：

| Worker | 理论批次数 |    理论时间 |
| -----: | ----: | ------: |
|      1 |    10 | ≈ 5.0 s |
|      2 |     5 | ≈ 2.5 s |
|      3 |     4 | ≈ 2.0 s |
|      4 |     3 | ≈ 1.5 s |
|      8 |     2 | ≈ 1.0 s |
|     16 |     1 | ≈ 0.5 s |

实际输出也验证了这一点。

例如 1 worker：

```text
worker 0 executing task
task 0 start
task 0 done
worker 0 executing task
task 1 start
task 1 done
...
```

任务完全串行。

而 4 workers：

```text
worker 0 executing task
task 0 start

worker 3 executing task
task 1 start

worker 1 executing task
task 3 start

worker 2 executing task
task 2 start
```

可以同时执行多个任务。

16 workers 时，由于只有 10 个任务，基本可以一次让全部任务进入执行状态。

---

# 11. 一个重要结论：线程不是越多越好

这个实验最重要的地方并不是：

> “16 个线程比 1 个线程快。”

而是：

> **线程数量需要与实际工作负载匹配。**

当：

```text
Worker 数 < 任务数
```

增加 Worker 通常能够提高并发度。

但是当：

```text
Worker 数 >= 实际可并行任务数
```

继续增加 Worker 就不会继续提高这个 workload 的吞吐量。

例如：

```text
10 个任务
16 workers
```

最多也只能同时执行 10 个任务。

剩下 6 个 Worker 没有任务，只能等待。

真实系统中还存在：

* 线程创建成本
* 调度成本
* 上下文切换
* CPU Cache 影响
* 锁竞争
* 内存占用

所以实际工程中不会简单地追求“线程越多越好”。

---

# 12. ThreadPool 与前面知识的串联

这一节实际上把前面的知识全部串起来了：

```text
RAII
 ↓
unique_ptr / move
 ↓
thread
 ↓
mutex
 ↓
lock_guard
 ↓
condition_variable
 ↓
Producer / Consumer
 ↓
Task Queue
 ↓
ThreadPool
```

其中：

```text
mutex
```

解决共享数据安全。

```text
condition_variable
```

解决等待与唤醒。

```text
queue
```

负责任务缓冲。

```text
thread
```

负责并发执行。

```text
RAII
```

负责资源生命周期。

最终组合成：

```text
                 ┌──────────────┐
                 │  Task Queue  │
                 └──────┬───────┘
                        │
             ┌──────────┼──────────┐
             ↓          ↓          ↓
          Worker 0   Worker 1   Worker 2
             │          │          │
             ↓          ↓          ↓
           Task       Task       Task
```

---

# 13. 对 AI Systems 的意义

ThreadPool 本身并不是 LLM 推理框架，但它体现的思想会大量出现在 AI Systems 中。

例如一个推理服务：

```text
Client Request
      ↓
Request Queue
      ↓
Scheduler
      ↓
Batch
      ↓
Inference Runtime
      ↓
GPU
```

CPU 侧可能需要：

* 接收请求
* 管理请求队列
* 调度任务
* 准备输入
* 管理内存
* 管理 GPU work
* 回收结果

这些系统都离不开：

```text
thread
mutex
condition_variable
queue
atomic
lock-free structure
```

所以现在学习 ThreadPool 的目的，并不是以后去手写一个线程池，而是建立：

> **高并发 C++ Runtime 的基本思维模型。**

以后阅读 llama.cpp、SGLang 或其他推理 Runtime 的代码时，看到线程、队列、同步原语，就能够理解它们承担的系统职责。

---

# 14. 本节最终认识

这次 ThreadPool 实验让我真正理解了：

1. ThreadPool 是线程复用机制。
2. Task Queue 用来缓存等待执行的任务。
3. mutex 保护共享任务队列。
4. condition_variable 负责 Worker 的等待和唤醒。
5. Worker 获取任务后应该尽快释放锁。
6. 任务执行不能放在临界区里面。
7. `stop_` + `notify_all()` + `join()` 构成线程池的安全关闭流程。
8. 线程数量决定并发能力，但线程越多并不意味着性能越好。
9. 条件变量的核心不是“唤醒”，而是“等待某个共享状态满足条件”。
10. ThreadPool 是前面线程、锁、条件变量、RAII 等知识的第一次完整组合。

---

## 15. Day 02 阶段性总结

到这里，Modern C++ 并发部分已经从：

```text
“会使用 std::thread”
```

推进到了：

```text
“理解一个基本并发 Runtime 是怎么组织起来的”
```

这是这一天真正需要达到的目标。

下一阶段不再继续深入 ThreadPool，而是转向 **Systems C++ / Linux 系统编程**，重点学习进程、线程、文件描述符、Socket、共享库、内存模型等 AI Systems 开发真正会接触到的系统基础。
