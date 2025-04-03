# Lock-Free MPMC Queue

A high-performance, lock-free Multi-Producer Multi-Consumer (MPMC) queue implementation optimized for high-frequency trading applications.

## Overview

This MPMC queue provides a fixed-size, pre-allocated memory region for efficient concurrent data exchange between multiple producer and consumer threads without locks. Unlike a standard ring buffer, this implementation focuses on ensuring fair work distribution among consumers, making it ideal for parallel processing in market data systems.

## Key Features

- **Lock-Free Implementation**: Uses atomic operations instead of locks for thread synchronization
- **Balanced Work Distribution**: Fair allocation of work across multiple consumer threads
- **Cache-Line Alignment**: Prevents false sharing between cores to maximize performance
- **Memory Pre-Faulting**: Avoids page faults during operation for consistent performance
- **Deterministic Behavior**: Predictable performance characteristics under load
- **ABA Problem Prevention**: Handles the ABA problem using sequence counters

## Design Goals

1. **Fairness**: Evenly distribute work across consumer threads
2. **Throughput**: Maximize the number of operations per second
3. **Latency**: Minimize operation latency and reduce tail latencies
4. **Scalability**: Maintain good performance as thread count increases

## Implementation Details

The MPMC queue will be implemented using a combination of:

1. **Array-Based Storage**: Fixed-size array for deterministic performance
2. **Sequence Counters**: To prevent the ABA problem and track slots
3. **Memory Barriers**: Carefully placed to ensure visibility between threads
4. **Compare-Exchange Operations**: For atomic updates to queue state

## Usage Example

```cpp
// Create an MPMC queue with capacity 1024
MPMCQueue<int, 1024> queue;

// Producer threads
auto producer = [&queue](int id) {
    for (int i = 0; i < 100; ++i) {
        int value = id * 1000 + i;
        while (!queue.enqueue(value)) {
            // Yield if queue is full
            std::this_thread::yield();
        }
    }
};

// Consumer threads
auto consumer = [&queue](int id) {
    int value;
    size_t count = 0;
    
    while (count < 200) { // Assuming 2 producers, each producing 100 items
        if (queue.dequeue(value)) {
            // Process value
            count++;
        } else {
            // Yield if queue is empty
            std::this_thread::yield();
        }
    }
};

// Launch threads
std::vector<std::thread> producers;
std::vector<std::thread> consumers;

for (int i = 0; i < 2; ++i) producers.emplace_back(producer, i);
for (int i = 0; i < 2; ++i) consumers.emplace_back(consumer, i);

// Join threads
for (auto& t : producers) t.join();
for (auto& t : consumers) t.join();
```

## Performance Goals

- **Throughput**: Target at least 50 million operations per second for small messages
- **Latency**: Target under 100ns per operation at the 99th percentile
- **Scalability**: Linear scaling up to the number of physical cores
- **Fairness**: Less than 20% variation in work distribution between consumer threads

## Comparison with Ring Buffer

The MPMC Queue will build upon lessons learned from the Ring Buffer implementation, with these key improvements:

1. **Fairness**: Better work distribution among consumer threads
2. **Contention Management**: Reduced contention under high load
3. **Cache Efficiency**: Improved cache utilization for better throughput
4. **Hybrid Architecture Awareness**: Optimizations for P-core/E-core systems

## Development Roadmap

1. **Implementation**: Core queue implementation with minimal functionality
2. **Testing**: Comprehensive test suite for correctness validation
3. **Benchmarking**: Performance analysis under various workloads
4. **Optimization**: Fine-tuning based on benchmark results
5. **Documentation**: Detailed documentation of design and implementation
6. **Integration**: Example usage in a market data processing system

## Build and Test

```bash
# Create build directory
mkdir build && cd build

# Generate build files with CMake
cmake ..

# Build the project
cmake --build . --config Release

# Run tests
ctest -C Release -V
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.