#include "../include/ring_buffer.h"
#include <benchmark/benchmark.h>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

// Single-threaded enqueue benchmark
static void BM_SingleThreadedEnqueue(benchmark::State& state) {
    // Create a ring buffer with the specified size
    const size_t buffer_size = state.range(0);
    RingBuffer<int, 1024> buffer; // Fixed size for benchmark
    
    for (auto _ : state) {
        state.PauseTiming();
        // Clear the buffer between iterations
        int value;
        while (buffer.try_dequeue(value)) {}
        state.ResumeTiming();
        
        // Benchmark enqueue operations
        for (size_t i = 0; i < buffer_size; ++i) {
            buffer.try_enqueue(static_cast<int>(i));
        }
    }
    
    state.SetItemsProcessed(state.iterations() * buffer_size);
}

// Single-threaded dequeue benchmark
static void BM_SingleThreadedDequeue(benchmark::State& state) {
    // Create a ring buffer with the specified size
    const size_t buffer_size = state.range(0);
    RingBuffer<int, 1024> buffer; // Fixed size for benchmark
    
    for (auto _ : state) {
        state.PauseTiming();
        // Fill the buffer before testing dequeue
        for (size_t i = 0; i < buffer_size; ++i) {
            buffer.try_enqueue(static_cast<int>(i));
        }
        state.ResumeTiming();
        
        // Benchmark dequeue operations
        int value;
        for (size_t i = 0; i < buffer_size; ++i) {
            buffer.try_dequeue(value);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * buffer_size);
}

// Multi-threaded producer-consumer benchmark
template<size_t BufferSize>
static void BM_MultiThreaded(benchmark::State& state) {
    // Number of items to process per iteration

    // For quick verification during development
    constexpr size_t num_items = 1000;  // 1K

    // For thorough validation (run separately)
    //constexpr size_t num_items = 100000;  // 1M
    
    // Number of producer and consumer threads
    const size_t num_producers = state.range(0);
    const size_t num_consumers = state.range(1);
    
    for (auto _ : state) {
        // Create a new buffer for each iteration
        RingBuffer<int, BufferSize> buffer;
        
        // Synchronization variables
        std::atomic<size_t> items_produced(0);
        std::atomic<size_t> items_consumed(0);
        std::atomic<bool> done(false);
        std::atomic<bool> start(false);
        
        // Producer function
        auto producer_func = [&](size_t producer_id) {
            // Wait for start signal
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            const size_t items_per_producer = num_items / num_producers;
            const size_t start_item = producer_id * items_per_producer;
            const size_t end_item = start_item + items_per_producer;
            
            for (size_t i = start_item; i < end_item; ++i) {
                while (!buffer.try_enqueue(static_cast<int>(i))) {
                    std::this_thread::yield();
                    // Check if we should terminate early
                    if (done.load(std::memory_order_acquire)) return;
                }
                items_produced.fetch_add(1, std::memory_order_relaxed);
            }
        };
        
        // Consumer function
        auto consumer_func = [&]() {
            // Wait for start signal
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            int value;
            while (items_consumed.load(std::memory_order_relaxed) < num_items) {
                if (buffer.try_dequeue(value)) {
                    items_consumed.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                    // Check if we should terminate early
                    if (done.load(std::memory_order_acquire) && 
                        items_produced.load(std::memory_order_acquire) == items_consumed.load(std::memory_order_acquire)) {
                        return;
                    }
                }
            }
        };
        
        // Create producer threads
        std::vector<std::thread> producers;
        producers.reserve(num_producers);
        for (size_t i = 0; i < num_producers; ++i) {
            producers.emplace_back(producer_func, i);
        }
        
        // Create consumer threads
        std::vector<std::thread> consumers;
        consumers.reserve(num_consumers);
        for (size_t i = 0; i < num_consumers; ++i) {
            consumers.emplace_back(consumer_func);
        }
        
        // Start the benchmark
        start.store(true, std::memory_order_release);
        
        // Wait for completion or timeout
        auto start_time = std::chrono::high_resolution_clock::now();
        while (items_consumed.load(std::memory_order_relaxed) < num_items) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
            if (elapsed > 5) { // 5 second timeout
                done.store(true, std::memory_order_release);
                break;
            }
            std::this_thread::yield();
        }
        
        // Signal completion and join threads
        done.store(true, std::memory_order_release);
        
        for (auto& t : producers) {
            if (t.joinable()) t.join();
        }
        
        for (auto& t : consumers) {
            if (t.joinable()) t.join();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_items);
    state.SetLabel(std::to_string(num_producers) + "p-" + std::to_string(num_consumers) + "c");
}

// Comparison with std::queue + mutex
static void BM_StdQueueWithMutex(benchmark::State& state) {
    const size_t buffer_size = state.range(0);
    std::queue<int> queue;
    std::mutex mutex;
    
    for (auto _ : state) {
        state.PauseTiming();
        // Clear the queue between iterations
        {
            std::lock_guard<std::mutex> lock(mutex);
            std::queue<int> empty;
            std::swap(queue, empty);
        }
        state.ResumeTiming();
        
        // Benchmark enqueue operations
        for (size_t i = 0; i < buffer_size; ++i) {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(static_cast<int>(i));
        }
        
        // Benchmark dequeue operations
        for (size_t i = 0; i < buffer_size; ++i) {
            std::lock_guard<std::mutex> lock(mutex);
            if (!queue.empty()) {
                queue.pop();
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations() * buffer_size * 2); // Enqueue + dequeue
}

// Register the benchmarks
BENCHMARK(BM_SingleThreadedEnqueue)->RangeMultiplier(2)->Range(64, 1024);
BENCHMARK(BM_SingleThreadedDequeue)->RangeMultiplier(2)->Range(64, 1024);
BENCHMARK(BM_StdQueueWithMutex)->RangeMultiplier(2)->Range(64, 1024);

// Multi-threaded benchmarks with different producer/consumer combinations
BENCHMARK_TEMPLATE(BM_MultiThreaded, 1024)->Args({1, 1});  // 1 producer, 1 consumer
BENCHMARK_TEMPLATE(BM_MultiThreaded, 1024)->Args({2, 2});  // 2 producers, 2 consumers
//BENCHMARK_TEMPLATE(BM_MultiThreaded, 1024)->Args({4, 4});  // 4 producers, 4 consumers
BENCHMARK_TEMPLATE(BM_MultiThreaded, 1024)->Args({1, 4});  // 1 producer, 4 consumers
//BENCHMARK_TEMPLATE(BM_MultiThreaded, 1024)->Args({4, 1});  // 4 producers, 1 consumer

// Different buffer sizes
BENCHMARK_TEMPLATE(BM_MultiThreaded, 64)->Args({2, 2});    // Small buffer
BENCHMARK_TEMPLATE(BM_MultiThreaded, 256)->Args({2, 2});   // Medium buffer
//BENCHMARK_TEMPLATE(BM_MultiThreaded, 1024)->Args({2, 2});  // Large buffer
BENCHMARK_TEMPLATE(BM_MultiThreaded, 4096)->Args({2, 2});  // Very large buffer

BENCHMARK_MAIN();