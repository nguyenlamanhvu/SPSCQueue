#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <pthread.h>
#include <iomanip>

#ifdef HAS_BOOST
#include <boost/lockfree/spsc_queue.hpp>
#endif

#include "SPSCQueue.h"

using namespace std;

struct UAVData {
    uint64_t id;
    double pitch, roll, yaw;
    float thrust;
    uint32_t status;
    UAVData(uint64_t i = 0, double p = 0, double r = 0, double y = 0, float t = 0, uint32_t s = 0)
        : id(i), pitch(p), roll(r), yaw(y), thrust(t), status(s) {}
};

void pinThread(int cpu) {
    if (cpu < 0) return;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

const size_t TOTAL_OPS = 10'000'000;
const size_t QUEUE_SIZE = 65536;

// --- WRAPPER INTERFACE ĐỂ ĐỒNG NHẤT CÁCH GỌI ---
template <typename T, typename QueueType>
struct QueueAdapter {
    static bool push(QueueType& q, uint64_t i) {
        return q.push(UAVData(i, 0.1, 0.2, 0.3, 0.5f, 1));
    }
    static bool pop(QueueType& q, T& data) {
        return q.pop(data);
    }
};

// Chuyên biệt hóa cho thư viện của bạn (Sử dụng emplace)
template <typename T>
struct QueueAdapter<T, SPSCQueue<T>> {
    static bool push(SPSCQueue<T>& q, uint64_t i) {
        return q.emplace(i, 0.1, 0.2, 0.3, 0.5f, 1);
    }
    static bool pop(SPSCQueue<T>& q, T& data) {
        return q.pop(data);
    }
};

// --- HÀM BENCHMARK GENERIC ĐÃ SỬA ---
template <typename QueueType>
void run_test(string name, int cpu1, int cpu2) {
    QueueType q(QUEUE_SIZE); 
    atomic<bool> start_signal{false};

    thread t1([&]() {
        pinThread(cpu1);
        UAVData data;
        while (!start_signal.load(memory_order_acquire));
        for (size_t i = 0; i < TOTAL_OPS; ++i) {
            while (!QueueAdapter<UAVData, QueueType>::pop(q, data));
        }
    });

    pinThread(cpu2);
    this_thread::sleep_for(chrono::seconds(1));
    auto start = chrono::steady_clock::now();
    start_signal.store(true, memory_order_release);

    for (size_t i = 0; i < TOTAL_OPS; ++i) {
        while (!QueueAdapter<UAVData, QueueType>::push(q, i));
    }
    t1.join();
    auto stop = chrono::steady_clock::now();
    
    auto duration = chrono::duration_cast<chrono::nanoseconds>(stop - start).count();
    cout << left << setw(20) << name 
         << " | Throughput: " << setw(10) << (TOTAL_OPS * 1000000) / duration << " ops/ms"
         << " | Latency: " << (double)duration / TOTAL_OPS << " ns/op" << endl;
}

int main(int argc, char* argv[]) {
    int cpu1 = 0, cpu2 = 2;
    if (argc == 3) {
        cpu1 = stoi(argv[1]);
        cpu2 = stoi(argv[2]);
    }

    cout << "Comparing performance on CPU " << cpu1 << " and " << cpu2 << endl;
    cout << "Payload size: " << sizeof(UAVData) << " bytes" << endl;
    cout << "----------------------------------------------------------------------" << endl;

    run_test<SPSCQueue<UAVData>>("SPSCQueue", cpu1, cpu2);

#ifdef HAS_BOOST
    run_test<boost::lockfree::spsc_queue<UAVData>>("Boost_SPSC", cpu1, cpu2);
#endif

    return 0;
}