#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "../include/mpmc_queue.h"

int main() {
    // Create a queue with 1024 elements capacity
    MPMCQueue<int, 1024> queue;
    
    std::cout << "MPMC Queue Implementation Demo\n";
    std::cout << "--------------------------------\n";
    
    // Basic operations demo
    std::cout << "Basic operations:\n";
    
    // Enqueue some items
    for (int i = 0; i < 5; ++i) {
        queue.enqueue(i);
        std::cout << "Enqueued: " << i << std::endl;
    }
    
    // Dequeue some items
    for (int i = 0; i < 3; ++i) {
        auto result = queue.dequeue();
        if (result) {
            std::cout << "Dequeued: " << result.value() << std::endl;
        }
    }
    
    std::cout << "Queue size: " << queue.size() << std::endl;
    
    // Multi-threaded demo
    std::cout << "\nMulti-threaded demo:\n";
    std::cout << "Running 4 producers and 4 consumers...\n";
    
    // Clear the queue
    int temp;
    while (queue.dequeue(temp)) {}
    
    // Number of items each producer will produce
    constexpr int ITEMS_PER_PRODUCER = 1000;
    constexpr int NUM_PRODUCERS = 4;
    constexpr int NUM_CONSUMERS = 4;
    
    // Counters
    std::atomic<int> produced(0);
    std::atomic<int> consumed(0);
    std::atomic<bool> done(false);
    
    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create producer threads
    std::vector<std::thread> producers;
    for (int p = 0; p < NUM_PRODUCERS; ++p) {
        producers.emplace_back([&queue, &produced, &done, p]() {
            for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
                // Create unique values by combining producer ID and item number
                int value = p * ITEMS_PER_PRODUCER + i;
                
                // Try to enqueue until successful
                while (!queue.enqueue(value)) {
                    // Check if we should exit
                    if (done.load(std::memory_order_relaxed)) {
                        return;
                    }
                    std::this_thread::yield();
                }
                
                produced.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // Create consumer threads
    std::vector<std::thread> consumers;
    for (int c = 0; c < NUM_CONSUMERS; ++c) {
        consumers.emplace_back([&queue, &consumed, &done]() {
            while (true) {
                int value;
                if (queue.dequeue(value)) {
                    consumed.fetch_add(1, std::memory_order_relaxed);
                    
                    // Check if we've consumed everything
                    if (consumed.load(std::memory_order_relaxed) >= NUM_PRODUCERS * ITEMS_PER_PRODUCER) {
                        break;
                    }
                } else {
                    // Check if we should exit
                    if (done.load(std::memory_order_relaxed) && 
                        consumed.load(std::memory_order_relaxed) >= produced.load(std::memory_order_relaxed)) {
                        break;
                    }
                    std::this_thread::yield();
                }
            }
        });
    }
    
    // Wait for producers to finish
    for (auto& p : producers) {
        p.join();
    }
    
    // Wait a bit for consumers to finish processing remaining items
    while (consumed.load(std::memory_order_relaxed) < produced.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        // Prevent infinite wait
        auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > 5) {
            std::cout << "Timeout waiting for consumers. Some items may remain in the queue.\n";
            done.store(true, std::memory_order_relaxed);
            break;
        }
    }
    
    // Signal consumers to exit and wait for them
    done.store(true, std::memory_order_relaxed);
    for (auto& c : consumers) {
        c.join();
    }
    
    // Calculate elapsed time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    // Print results
    std::cout << "Completed in " << duration << " ms\n";
    std::cout << "Items produced: " << produced.load() << "\n";
    std::cout << "Items consumed: " << consumed.load() << "\n";
    std::cout << "Items remaining in queue: " << queue.size() << "\n";
    std::cout << "Throughput: " << (produced.load() * 1000.0 / duration) << " items/second\n";
    
    return 0;
}
