#include "../include/ring_buffer.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <iomanip>

// Simple benchmarking helper
class Timer {
public:
    Timer(const std::string& name) : name_(name), start_(std::chrono::high_resolution_clock::now()) {}
    
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << name_ << ": " << duration << " microseconds" << std::endl;
    }
    
private:
    std::string name_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// Test single-threaded performance
void test_single_threaded() {
    constexpr size_t buffer_size = 1024;
    constexpr size_t num_operations = 1000000;
    
    std::cout << "\n=== Single-Threaded Performance Test ===\n";
    
    // Create ring buffer
    RingBuffer<int, buffer_size> buffer;
    
    // Test enqueue performance
    {
        Timer timer("Enqueue " + std::to_string(num_operations) + " items");
        for (size_t i = 0; i < num_operations && !buffer.full(); ++i) {
            buffer.try_enqueue(static_cast<int>(i));
        }
    }
    
    // Test dequeue performance
    {
        Timer timer("Dequeue " + std::to_string(num_operations) + " items");
        int value;
        for (size_t i = 0; i < num_operations && buffer.try_dequeue(value); ++i) {
            // Just consume the value
        }
    }
}

// Test multi-threaded performance
void test_multi_threaded() {
    constexpr size_t buffer_size = 1024;
    constexpr size_t num_operations = 10000000;
    constexpr size_t num_producers = 2;
    constexpr size_t num_consumers = 2;
    
    std::cout << "\n=== Multi-Threaded Performance Test ===\n";
    std::cout << "Producers: " << num_producers << ", Consumers: " << num_consumers << "\n";
    
    // Create ring buffer
    RingBuffer<int, buffer_size> buffer;
    
    // Synchronization flags
    std::atomic<bool> start_flag(false);
    std::atomic<bool> stop_flag(false);
    std::atomic<size_t> total_produced(0);
    std::atomic<size_t> total_consumed(0);
    
    // Producer function
    auto producer_func = [&](int producer_id) {
        // Wait for start signal
        while (!start_flag.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        
        size_t count = 0;
        while (!stop_flag.load(std::memory_order_acquire) && count < num_operations / num_producers) {
            // Try to enqueue an item
            if (buffer.try_enqueue(static_cast<int>(count))) {
                count++;
                total_produced.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    };
    
    // Consumer function
    auto consumer_func = [&](int consumer_id) {
        // Wait for start signal
        while (!start_flag.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        
        int value;
        size_t count = 0;
        while (!stop_flag.load(std::memory_order_acquire) || !buffer.empty()) {
            // Try to dequeue an item
            if (buffer.try_dequeue(value)) {
                count++;
                total_consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    };
    
    // Create producer and consumer threads
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    
    for (size_t i = 0; i < num_producers; ++i) {
        producers.emplace_back(producer_func, i);
    }
    
    for (size_t i = 0; i < num_consumers; ++i) {
        consumers.emplace_back(consumer_func, i);
    }
    
    // Start the test
    auto start_time = std::chrono::high_resolution_clock::now();
    start_flag.store(true, std::memory_order_release);
    
    // Let it run for a fixed duration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Signal stop
    stop_flag.store(true, std::memory_order_release);
    
    // Join all threads
    for (auto& t : producers) {
        if (t.joinable()) t.join();
    }
    
    for (auto& t : consumers) {
        if (t.joinable()) t.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    
    // Calculate throughput
    double ops_per_sec = static_cast<double>(total_consumed.load()) / (duration / 1000000.0);
    
    std::cout << "Total duration: " << duration / 1000.0 << " ms\n";
    std::cout << "Items produced: " << total_produced.load() << "\n";
    std::cout << "Items consumed: " << total_consumed.load() << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Throughput: " << ops_per_sec / 1000000.0 << " million ops/sec\n";
}

int main() {
    std::cout << "=== Ring Buffer Performance Tests ===\n";
    std::cout << "Compiled with C++ " << __cplusplus << "\n";
    std::cout << "Cache line size: " << CACHE_LINE_SIZE << " bytes\n";
    std::cout << "Hardware threads: " << std::thread::hardware_concurrency() << "\n";
    
    // Run tests
    test_single_threaded();
    test_multi_threaded();
    
    return 0;
}
