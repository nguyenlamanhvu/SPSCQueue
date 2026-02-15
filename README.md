# High-Performance SPSC Queue for UAV Control Systems

A lock-free, ultra-low latency Single-Producer Single-Consumer (SPSC) queue implementation in C++, specifically optimized for real-time Quadrotor flight control systems.

## 🚀 Key Features

- **Zero-copy Emplace**: Constructs objects directly within the queue's memory using `placement new`, eliminating unnecessary data copy overhead.
- **Cache-line Alignment**: Leverages `alignas(64)` to prevent *False Sharing*, ensuring optimal L1/L2 cache utilization.
- **Cached Indices**: Minimizes memory bus contention by keeping local copies of head/tail indices.
- **Hybrid Architecture Optimized**: Specifically tuned for modern CPUs with hybrid architectures (Intel P-cores and E-cores).
- **Extreme Performance**: Outperforms industry standards like Boost.Lockfree by ~50-100% in latency-sensitive scenarios.

## 📊 Benchmark Results (Head-to-Head)

Tested on **Ubuntu 24.04 LTS**, CPU: **Intel Core Ultra 5 125H (Meteor Lake)**.  
Payload size: **40 bytes** (`UAVData` struct).

### 1. Throughput & Latency Comparison
| Scenario (Core ID) | Library | Throughput (ops/ms) | Avg Latency | Improvement |
| :--- | :--- | :--- | :--- | :--- |
| **P-core to P-core (0 -> 1)** | **SPSCQueue** | **222,115** | **4.50 ns** | **+57%** |
| | Boost_SPSC | 141,263 | 7.07 ns | Baseline |
| **P-core to P-core (0 -> 2)** | **SPSCQueue** | **205,009** | **4.87 ns** | **+40%** |
| | Boost_SPSC | 146,530 | 6.82 ns | Baseline |
| **P-core to E-core (0 -> 8)** | **SPSCQueue** | **170,725** | **5.85 ns** | **+120%** |
| | Boost_SPSC | 77,462 | 12.90 ns | Baseline |

### 2. Analysis
- **P-core (0 -> 1)**: Lowest latency achieved due to shared L1/L2 cache on the same physical core (Hyper-threading).
- **P-core to E-core (0 -> 8)**: Your implementation shows massive resilience to cross-cluster communication overhead compared to Boost, maintaining sub-6ns latency.

## 🛠 Installation & Usage

### Prerequisites
- C++17 compliant compiler (GCC 11+ or Clang 14+).
- **Boost Library**: `sudo apt-get install libboost-all-dev` (Optional, for comparison).

### Build the Library & Benchmark
This is a header-only library. To build the provided benchmark:
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)

### Run the Benchmark
```bash
./spsc_benchmark 0 2