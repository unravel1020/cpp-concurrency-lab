# Pipe：从 File Descriptor 到进程间通信

## 1. 为什么学习 pipe

前一节学习了 File Descriptor。

Linux 中：

```text
FD 0 → stdin
FD 1 → stdout
FD 2 → stderr
```

并且通过：

```cpp
open()
write()
read()
close()
```

可以操作不同的 I/O 对象。

这一节进一步学习：

```cpp
pipe()
```

让两个进程通过 FD 进行通信。

---

## 2. pipe 的基本模型

调用：

```cpp
int pipefd[2];

pipe(pipefd);
```

会得到两个文件描述符：

```text
pipefd[0] → read end
pipefd[1] → write end
```

数据流向：

```text
write(fd[1])
      ↓
┌────────────┐
│    pipe    │
└────────────┘
      ↓
read(fd[0])
```

因此 pipe 本质上提供了一个内核维护的字节流缓冲区。

---

## 3. pipe 与 fork 的组合

这次实验采用：

```text
pipe()
  ↓
fork()
```

而不是：

```text
fork()
  ↓
pipe()
```

原因是 `fork()` 会让子进程继承父进程已经打开的 FD。

因此：

```text
fork() 前

Parent
├── FD 3 → pipe read
└── FD 4 → pipe write
```

执行 `fork()` 后：

```text
Parent                         Child
├── FD 3 → pipe read           ├── FD 3 → pipe read
└── FD 4 → pipe write          └── FD 4 → pipe write
```

父子进程拥有指向同一个 pipe 的 FD。

于是就可以实现：

```text
Parent
   │
   │ write()
   ↓
 pipe
   │
   │ read()
   ↓
Child
```

---

## 4. 为什么要关闭不使用的 FD

父进程只负责写：

```cpp
close(pipefd[0]);
```

子进程只负责读：

```cpp
close(pipefd[1]);
```

最终：

```text
Parent
   │
   │ write end
   ↓
┌──────────┐
│   pipe   │
└──────────┘
   │
   │ read end
   ↓
Child
```

这样可以明确通信方向，并且对 EOF 的判断非常重要。

---

## 5. 第一次实验：父进程发送数据

父进程：

```cpp
const char* message =
    "hello from parent";

write(
    pipefd[1],
    message,
    std::strlen(message));
```

子进程：

```cpp
ssize_t n = read(
    pipefd[0],
    buffer,
    sizeof(buffer) - 1);
```

实际输出：

```text
read fd  = 3
write fd = 4
child received: hello from parent
```

说明父进程写入的数据成功通过 pipe 到达子进程。

---

## 6. 第二次实验：read 的阻塞

让父进程先：

```cpp
sleep(5);
```

然后才执行：

```cpp
write(...)
```

此时子进程已经执行：

```cpp
read(...)
```

但是 pipe 中暂时没有数据。

因此：

```text
Child
  ↓
read()
  ↓
pipe 没有数据
  ↓
阻塞
```

5 秒后：

```text
Parent
  ↓
write()
  ↓
pipe 有数据
  ↓
Child read() 返回
```

这说明默认情况下 `read()` 是阻塞式的。

也就是说：

> 没有数据时，调用线程可以等待，而不是不断轮询。

---

## 7. 第三次实验：EOF

最后一次实验中，父进程不写任何数据：

```cpp
close(pipefd[1]);
```

子进程执行：

```cpp
ssize_t n = read(
    pipefd[0],
    buffer,
    sizeof(buffer) - 1);
```

实际输出：

```text
read fd  = 3
write fd = 4
read returned: 0
```

这里：

```text
read() > 0
```

表示读取到了数据。

```text
read() == 0
```

表示 EOF。

```text
read() < 0
```

表示发生错误。

所以：

> `read() == 0` 并不是简单的“这次没有数据”。

对于 pipe，它意味着：

> **写端已经全部关闭，并且 pipe 中已经没有剩余数据。**

---

## 8. “没有数据”和 EOF 的区别

这是本节最重要的一个区别。

### 情况 A：暂时没有数据

```text
pipe
 │
 └── 空
```

写端仍然存在：

```text
write end → open
```

此时：

```cpp
read()
```

可能阻塞等待。

---

### 情况 B：EOF

```text
pipe
 │
 └── 空
```

并且：

```text
所有 write end → close
```

此时：

```cpp
read()
```

返回：

```text
0
```

因此：

```text
没有数据 ≠ EOF
```

EOF 的核心条件是：

```text
没有数据
+
所有写端关闭
```

---

## 9. FD、fork、pipe 三者的关系

这次实验实际上把两个前面学过的概念连接起来了：

```text
                pipe()
                  ↓
              得到两个 FD
                  ↓
                fork()
                  ↓
          ┌───────┴───────┐
          ↓               ↓
       Parent            Child
          │               │
       write()          read()
          │               │
          └──── pipe ─────┘
```

所以：

```text
FD
```

不只是用于普通文件。

它同样可以表示：

```text
文件
Terminal
pipe
Socket
设备
```

这正是 Linux I/O 抽象的重要特点。

---

## 10. 对 Systems C++ 的意义

现在可以建立一个比较完整的系统模型：

```text
Process
   │
   ├── Threads
   │
   └── File Descriptors
           │
           ├── File
           ├── Terminal
           ├── Pipe
           ├── Socket
           └── Device
```

而不同进程之间可以利用这些 I/O 对象进行通信。

例如：

```text
Process A
   │
   │ pipe
   ↓
Process B
```

或者：

```text
Client
   │
   │ Socket
   ↓
Server
```

这也是后面学习 Linux 网络编程的基础。

---

# 11. 本节最终认识

通过实际实验，我理解了：

1. `pipe()` 创建一个单向通信通道。
2. `pipefd[0]` 是读端。
3. `pipefd[1]` 是写端。
4. `fork()` 后，子进程会继承父进程的 FD。
5. 父子进程因此可以共享同一个 pipe。
6. `write()` 把数据写入 pipe。
7. `read()` 从 pipe 读取数据。
8. 默认情况下，没有数据时 `read()` 可以阻塞。
9. `read() == 0` 表示 EOF。
10. “暂时没有数据”和“EOF”是两个不同的状态。
11. pipe 是 Linux 中最基础的进程间通信机制之一。

这次实验让我第一次真正看到：

```text
FD
 +
fork
 +
pipe
 =
进程间通信
```

这比单独学习 `pipe()` API 更重要。
