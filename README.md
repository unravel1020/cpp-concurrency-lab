# cpp-concurrency-lab

现代 C++ 并发实验项目（C++17 / CMake / GoogleTest / Google Benchmark）。

> 目标：把「C++ 基础使用能力」提升为「现代 C++ 工程能力」，
> 并用真实 benchmark 回答：**为什么线程数增加后性能不会无限提升？**

## 结构

```
cpp-concurrency-lab/
├── CMakeLists.txt
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

## 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build            # 跑单元测试
./build/pool_bench                # 跑基准
```

## 路线图

- [x] ThreadPool + BlockingQueue（C++17, RAII, move semantics）
- [ ] GDB 调试实战记录
- [ ] 多线程 benchmark（1/2/4/8/16 workers × 不同任务粒度）
- [ ] 性能分析：锁竞争 / 缓存局部性 / false sharing
