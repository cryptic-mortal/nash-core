# MCCFR High-Performance Poker Solver

[![C++](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://isocpp.org/)
[![Performance](https://img.shields.io/badge/Performance-6.3M%20iters%2Fs-green.svg)](#key-metrics)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A high-performance implementation of **Monte Carlo Counterfactual Regret Minimization (MCCFR)** designed for the Leduc Hold'em poker variant. This solver leverages modern C++20 features, lock-free concurrency, and flattened game tree structures to achieve millions of iterations per second on consumer hardware.

---

## 🚀 Key Features

- **High-Frequency Training:** Processes over 6.3 Million game iterations per second on a quad-core setup.
- **Parallel Scalability:** Multi-threaded solver using `std::atomic_ref` for lock-free, relaxed memory-order updates.
- **Optimized Game Trees:** Pre-built, flattened game tree nodes in RAM to eliminate runtime branching and allocation overhead.
- **Precise Analytics:** Real-time exploitability logging and scalability benchmarking.
- **Modern C++ Stack:** Built with C++20, utilizing hardware concurrency and advanced memory models.

---

## 📈 Key Metrics

Benchmarks conducted on a **50 Million Iteration** workload.

| Threads | Time (s) | Speedup S(N) | Efficiency E(N) | Throughput (iters/s) |
| :--- | :--- | :--- | :--- | :--- |
| **1** | 26.606 | 1.00x | 100.0% | 1,879,296 |
| **2** | 15.671 | 1.70x | 84.9% | 3,190,598 |
| **4** | 7.852 | **3.39x** | **84.7%** | **6,367,569** |
| **8** | 12.417 | 2.14x | 26.8% | 4,026,745 |

> **Note:** Performance peaks at 4 physical cores on the M2 architecture due to the efficiency/performance core split and cache contention at high thread counts.

---

## 💻 System Specifications

- **Device:** MacBook Air M2 (2022)
- **Processor:** Apple M2 (8-core CPU)
- **Memory:** 8 GB Unified RAM
- **OS:** macOS

---

## 🛠️ Build & Run

### Compiler Flags
The project requires a C++20 compliant compiler. We use aggressive optimization and target the native architecture for maximum throughput.

```bash
# Compilation flags
FLAGS="-O3 -std=c++20 -march=native -ffast-math"
```

### Compile & Run Benchmark
```bash
g++ $FLAGS src/benchmark.cpp src/cfr_solver.cpp src/game_tree.cpp src/game_kernel.cpp -Iinclude -o bin/benchmark
./bin/benchmark
```

### Compile & Run Solver
```bash
g++ $FLAGS src/cfr_solver.cpp src/game_tree.cpp src/game_kernel.cpp -Iinclude -o bin/solver
./bin/solver
```

---

## 🏗️ Architecture

1. **Indexer:** Maps complex game states (private card, public card, history) to unique `uint32_t` IDs.
2. **Game Kernel:** A pure functional implementation of Leduc Hold'em rules.
3. **Game Tree:** A static, in-memory representation of the entire state space (840 Information Sets).
4. **CFR Solver:**
   - Uses **External Sampling** MCCFR.
   - Updates `global_regret_sum` and `global_strategy_sum` using atomic operations.
   - Batch-processing to minimize cache coherency traffic between cores.

---

## 📜 License

Distributed under the MIT License. See `LICENSE` for more information.
