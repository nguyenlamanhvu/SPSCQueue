#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <vector>
#include <pthread.h>
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

// Function to pin a thread to a specific CPU core
void pinThread(int cpu) {
    if (cpu < 0) return;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
        perror("pthread_setaffinity_np");
        exit(1);
    }
}

const size_t TOTAL_OPS = 10'000'000;

void run_benchmarks(int cpu1, int cpu2) {
    SPSCQueue<UAVData> q1(65536);
    SPSCQueue<UAVData> q2(65536);
    atomic<bool> start_signal{false};

    // --- TEST 1: THROUGHPUT ---
    cout << "Testing Throughput (CPU " << cpu1 << " -> " << cpu2 << ")..." << endl;
    
    thread t1([&]() {
        pinThread(cpu1); // Thread (Consumer)
        UAVData data;
        while (!start_signal.load(memory_order_acquire));
        for (size_t i = 0; i < TOTAL_OPS; ++i) {
            while (!q1.pop(data));
        }
    });

    pinThread(cpu2); // Thread (Producer)
    this_thread::sleep_for(chrono::seconds(1)); // Warm up
    
    auto start = chrono::steady_clock::now();
    start_signal.store(true, memory_order_release);
    
    for (size_t i = 0; i < TOTAL_OPS; ++i) {
        while (!q1.emplace(i, 0.1, 0.2, 0.3, 0.5f, 1));
    }
    t1.join();
    auto stop = chrono::steady_clock::now();
    
    auto duration_ns = chrono::duration_cast<chrono::nanoseconds>(stop - start).count();
    cout << "Throughput: " << (TOTAL_OPS * 1000000) / duration_ns << " ops/ms" << endl;
    cout << "Latency:    " << (double)duration_ns / TOTAL_OPS << " ns/op" << endl;

    // --- TEST 2: RTT (ROUND TRIP TIME) ---
    cout << "\nTesting RTT (Round Trip Latency)..." << endl;
    start_signal.store(false);
    
    thread t2([&]() {
        pinThread(cpu1);
        UAVData data;
        for (size_t i = 0; i < TOTAL_OPS; ++i) {
            while (!q1.pop(data));   // receive 
            while (!q2.emplace(data)); // send 
        }
    });

    pinThread(cpu2);
    UAVData recv_data;
    auto rtt_start = chrono::steady_clock::now();
    
    for (size_t i = 0; i < TOTAL_OPS; ++i) {
        while (!q1.emplace(i, 0.0, 0.0, 0.0, 0.0f, 0)); // send request
        while (!q2.pop(recv_data));                     // receive response
    }
    auto rtt_stop = chrono::steady_clock::now();
    t2.join();

    auto rtt_duration_ns = chrono::duration_cast<chrono::nanoseconds>(rtt_stop - rtt_start).count();
    cout << "Average RTT: " << (double)rtt_duration_ns / TOTAL_OPS << " ns" << endl;
}

int main(int argc, char* argv[]) {
    int cpu1 = 0; // P-Core default
    int cpu2 = 1; // P-Core default
    
    if (argc == 3) {
        cpu1 = stoi(argv[1]);
        cpu2 = stoi(argv[2]);
    }

    run_benchmarks(cpu1, cpu2);
    return 0;
}