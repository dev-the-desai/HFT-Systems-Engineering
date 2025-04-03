# MPMC Queue

A high-performance, lock-free Multiple-Producer Multiple-Consumer queue implementation optimized for High-Frequency Trading systems.

## Overview

This lock-free MPMC queue implementation provides a bounded buffer that can safely be accessed by multiple threads simultaneously, without using traditional locking mechanisms. The implementation is optimized for low-latency and high-throughput environments, such as High-Frequency Trading applications.

Key features:
- Fully lock-free design
- Cache-line aligned counters to prevent false sharing
- Bounded queue with a fixed capacity (power of 2)
- Strong memory ordering guarantees
- Optimized for both throughput and latency

## Implementation Details

The queue design is based on a sequence counter approach. Each slot in the queue has an associated atomic sequence counter that indicates whether the slot is available for producers or consumers. This design eliminates the ABA problem common in lock-free algorithms.

The implementation uses C++20 atomic operations with carefully selected memory ordering constraints to ensure correctness while minimizing overhead.

## API

```cpp
// Create an MPMC queue with a capacity of 1024 elements
MPMCQueue<int, 1024> queue;

// Enqueue an element
bool success = queue.enqueue(42);

// Dequeue an element (reference version)
int value;
bool success = queue.dequeue(value);

// Dequeue an element (optional version)
std::optional<int> result = queue.dequeue();
if (result) {
    int value = result.value();
}

// Queue status
bool isEmpty = queue.empty();
size_t numElements = queue.size();
constexpr size_t capacity = queue.capacity();
```

## Performance

The MPMC queue implementation is designed to provide excellent performance in both single-threaded and multi-threaded scenarios:

- Low-latency enqueue/dequeue operations
- High throughput under contention
- Minimal impact on cache coherency through careful alignment
- Fair scheduling for multiple producers and consumers

Performance can be evaluated using the included benchmark suite, which compares this implementation against traditional mutex-based queues.

## Requirements

- C++20 compatible compiler
- CMake 3.16 or newer

## Building

```bash
# Create a build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run tests
ctest
```

## Example

A simple example demonstrating the MPMC queue in a multi-producer, multi-consumer scenario:

```cpp
#include "mpmc_queue.h"
#include <thread>
#include <vector>

int main() {
    // Create a queue with capacity for 1024 elements
    MPMCQueue<int, 1024> queue;

    // Create multiple producer threads
    std::vector<std::thread> producers;
    for (int i = 0; i < 4; ++i) {
        producers.emplace_back([&queue, i]() {
            for (int j = 0; j < 1000; ++j) {
                // Enqueue items
                queue.enqueue(i * 1000 + j);
            }
        });
    }

    // Create multiple consumer threads
    std::vector<std::thread> consumers;
    for (int i = 0; i < 4; ++i) {
        consumers.emplace_back([&queue]() {
            int value;
            for (int j = 0; j < 1000; ++j) {
                // Dequeue items
                while (!queue.dequeue(value)) {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Wait for completion
    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    return 0;
}
```

## License

This code is provided for educational purposes as part of the High-Frequency Trading Systems Engineering course.
