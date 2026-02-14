# High-Performance SPSC Queue for UAV Control Systems

A lock-free, ultra-low latency Single-Producer Single-Consumer (SPSC) queue implementation in C++, specifically optimized for real-time Quadrotor flight control systems.

## 🚀 Key Features

- **Zero-copy Emplace**: Constructs objects directly within the queue's memory using `placement new`, eliminating unnecessary data copy overhead.
- **Cache-line Alignment**: Leverages `alignas(64)` to prevent *False Sharing*, ensuring optimal L1/L2 cache utilization.
- **Cached Indices**: Minimizes memory bus contention by keeping local copies of head/tail indices.
- **Hybrid Architecture Optimized**: Specifically tuned for modern CPUs with hybrid architectures (Intel P-cores and E-cores).
- **Extreme Performance**: Achieves ~4.2ns/op latency and >230 million ops/s throughput on Intel Core Ultra 5 (Meteor Lake).

## 📊 Benchmark Results

Tested on **Ubuntu 24.04 LTS**, CPU: **Intel Core Ultra 5 125H**.

| Scenario | Throughput (ops/ms) | Avg Latency | RTT (Round Trip) |
| :--- | :--- | :--- | :--- |
| **P-core to P-core** | ~106,000 ops/ms | ~9.3 ns | ~220 ns |
| **P-core to E-core** | ~89,000 ops/ms | ~11.1 ns | ~240 ns |

> **Note**: Peak performance is achieved using Real-time priority (`SCHED_FIFO`) and explicit Thread Pinning.

## 🛠 Installation & Usage

### Prerequisites
- C++17 compliant compiler (GCC 11+ or Clang 14+).
- Linux OS (required for `pthread_setaffinity_np` and `chrt`).

### Build the Benchmark
```bash
mkdir build && cd build
make -j$(nproc)

### Run the Benchmark
```bash
./spsc_benchmark 0 2