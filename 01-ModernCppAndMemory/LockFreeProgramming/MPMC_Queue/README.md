# Lock-Free MPMC Queue

A high-performance, lock-free Multi-Producer Multi-Consumer (MPMC) queue implementation optimized for high-frequency trading applications.

## Overview

This MPMC queue provides a fixed-size, pre-allocated memory region for efficient concurrent data exchange between multiple producer and consumer threads without locks. It is specifically designed for scenarios where multiple threads need to share data with minimal latency overhead and balanced work distribution, such as market data processing or parallel execution systems.

## Key Features

- **Lock-Free Implementation**: Uses atomic operations instead of locks for thread synchronization
- **Balanced Work Distribution**: Fair allocation of work across multiple consumer threads
- **Cache-Line Alignment**: Prevents false sharing between cores to maximize performance
- **Power-of-2 Sizing**: Enables fast index calculations via bitwise operations
- **ABA Problem Prevention**: Uses sequence counters to prevent the ABA problem
- **Memory Pre-Faulting**: Avoids page faults during operation for consistent performance
- **Comprehensive Test Suite**: Validates correctness in various scenarios
- **Extensive Benchmarking**: Compares performance against standard library alternatives

## Implementation Details

### Memory Ordering

The implementation carefully controls memory ordering semantics to ensure correct behavior in concurrent scenarios:

- `std::memory_order_relaxed` for initial reads where sequential consistency isn't required
- `std::memory_order_acquire` when reading values that other threads may have updated
- `std::memory_order_release` when writing values that other threads need to see
- Compare-exchange operations for ensuring atomicity of check-and-update operations

### Thread Safety

The MPMC queue is thread-safe for:
- Multiple producers / multiple consumers (true MPMC)
- Zero-contention operations on separate slots
- Fair work distribution among consumer threads
- Atomic operations ensuring correct visibility across cores

## Performance

### Single-Threaded Performance

| Operation | Buffer Size | Operations/sec | Comparison to std::queue+mutex |
|-----------|------------|----------------|-------------------------------|
| Enqueue   | 64         | ~97.5M/sec     | ~11x faster                   |
| Enqueue   | 1024       | ~33.4M/sec     | ~3.2x faster                  |
| Dequeue   | 64         | ~22.1M/sec     | ~2.5x faster                  |
| Dequeue   | 1024       | ~32.7M/sec     | ~3.2x faster                  |

### Multi-Threaded Performance

| Configuration    |  Items/sec     | Notes             |
|------------------|----------------|-------------------|
| 1p-1c (1024)     |  1.73M/s       | 1 producer, 1 consumer |
| 2p-2c (1024)     |  1.71M/s       | 2 producers, 2 consumers |
| 4p-4c (1024)     |  1.20M/s       | 4 producers, 4 consumers |
| 1p-4c (1024)     |  1.45M/s       | 1 producer, 4 consumers |
| 4p-1c (1024)     |  1.09M/s       | 4 producers, 1 consumer |
| 2p-2c (64)       |  1.49M/s       | 2 producers, 2 consumers, small buffer |
| 2p-2c (256)      |  1.63M/s       | 2 producers, 2 consumers, medium buffer |
| 2p-2c (4096)     |  1.99M/s       | 2 producers, 2 consumers, large buffer |

## Usage Example

```cpp
// Create an MPMC queue with capacity 1024
MPMCQueue<int, 1024> queue;

// Producer thread
auto producer = [&queue](int id) {
    for (int i = 0; i < 100; ++i) {
        int value = id * 1000 + i;
        while (!queue.enqueue(value)) {
            // Optionally yield if queue is full
            std::this_thread::yield();
        }
    }
};

// Consumer thread
auto consumer = [&queue](int id) {
    int value;
    size_t count = 0;
    
    while (count < 200) { // Assuming 2 producers, each producing 100 items
        if (queue.dequeue(value)) {
            // Process value
            count++;
        } else {
            // Optionally yield if queue is empty
            std::this_thread::yield();
        }
    }
};

// Launch threads
std::thread p1(producer, 0);
std::thread p2(producer, 1);
std::thread c1(consumer, 0);
std::thread c2(consumer, 1);

p1.join(); p2.join();
c1.join(); c2.join();
```

## Limitations and Trade-offs

- **Fixed Capacity**: Queue size must be known at compile time
- **Power-of-2 Restriction**: Capacity must be a power of 2 for optimal performance
- **Memory Usage**: Pre-allocates memory for the entire queue capacity
- **Scaling**: Performance doesn't scale linearly with thread count due to contention

## Future Improvements

- **Adaptive Backoff Strategy**: Implement exponential backoff for high-contention scenarios
- **NUMA Awareness**: Optimize for non-uniform memory architectures
- **Batching Operations**: Add support for processing multiple items at once
- **Custom Memory Allocator Integration**: Allow custom allocators for more control
- **Profiling Extensions**: Add hooks for detailed performance monitoring

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

## Benchmarking

```bash
# Run benchmarks
./mpmc_queue_bench
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.