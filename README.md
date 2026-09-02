# cpp-concurrency-lab

现代 C++ 并发实验项目（C++17 / CMake / GoogleTest / Google Benchmark）。

> 目标：把「C++ 基础使用能力」提升为「现代 C++ 工程能力」，
> 并用真实的 benchmark 回答：**为什么线程数增加后性能不会无限提升？**

## 结构

```
cpp-concurrency-lab/
├── CMakeLists.txt
├── CMakePresets.json          # dev / release 预设（clangd 友好）
├── src/
│   ├── blocking_queue.hpp     # 线程安全有界队列（RAII + condition_variable）
│   └── thread_pool.hpp        # 线程池（move semantics + std::future）
├── examples/
│   └── demo.cpp               # 最小可运行示例
├── tests/
│   └── thread_pool_test.cpp   # GoogleTest 单元测试
├── benchmarks/
│   └── pool_bench.cpp         # 线程数 / 队列深度 / 任务粒度对比基准
└── docs/
    └── analysis.md            # benchmark 结果与性能分析
```

## 开发环境（WSL 推荐）

```bash
sudo apt install clangd clang clang-format clang-tidy cmake ninja-build gdb g++ \
                 libgtest-dev libbenchmark-dev
```

VSCode 打开本目录（WSL Remote）后装 clangd 扩展即可获得补全/诊断/跳转；
`.clangd` 指向 `build/` 的 compile_commands.json（CMake 自动导出）。

## 构建

```bash
cmake --preset dev              # Debug + Ninja + compile_commands.json
cmake --build build
ctest --test-dir build          # 跑单元测试
./build/pool_bench              # 跑基准

cmake --preset release          # Release 版
cmake --build build-release
```

## 每日实验（dayNN/ 快速测试）

按计划每天一个 `.cpp` 丢进 `dayNN/`（如 `day01/raii.cpp`），CMake 自动注册为同名可执行文件：

```bash
cmake -S . -B build      # 首次配置 / 新增 dayNN 文件后重跑一次（自动重新扫描）
cmake --build build      # 编译（自动增量）
./build/raii             # 运行（目标名 = 文件名，Linux/WSL 下）
```

改完代码只需：

```bash
cmake --build build && ./build/raii
```

不用手动 `g++ xxx.cpp`，保持 out-of-source 构建习惯，也和 AI Systems 工程习惯一致。

## 路线图

- [x] ThreadPool + BlockingQueue（C++17, RAII, move semantics）
- [ ] GDB 调试实战记录
- [ ] 多线程 benchmark（1/2/4/8/16 workers × 不同任务粒度）
- [ ] 性能分析：锁竞争 / 缓存局部性 / false sharing
