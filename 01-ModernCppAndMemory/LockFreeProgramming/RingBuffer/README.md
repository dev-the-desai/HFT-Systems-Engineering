# Lock-Free Ring Buffer

A high-performance, lock-free circular buffer (ring buffer) implementation optimized for low-latency producer-consumer communication in trading systems.

## Overview

This ring buffer provides a fixed-size, pre-allocated memory region for efficient data exchange between threads without using locks. It is designed for scenarios where multiple producers and consumers need to share data with minimal latency overhead, such as market data processing or order execution systems.

## Key Features

- **Lock-Free Implementation**: Uses atomic operations instead of locks for thread synchronization
- **Cache-Line Alignment**: Prevents false sharing between cores to maximize performance
- **Power-of-2 Sizing**: Enables fast index calculations via bitwise operations
- **Move Semantics Support**: Efficiently handles both primitive types and complex objects
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

The ring buffer is thread-safe for:
- Multiple producers / multiple consumers
- Zero-contention operations on separate ends of the buffer
- Atomic operations ensuring correct visibility across cores

Note: In high-contention scenarios, work distribution may be uneven among consumer threads, with some threads processing more items than others.

## Performance

### Single-Threaded Performance

| Operation | Buffer Size | Operations/sec | Comparison to std::queue+mutex |
|-----------|------------|----------------|-------------------------------|
| Enqueue   | 64         | ~127M/sec      | ~13x faster                   |
| Enqueue   | 1024       | ~286M/sec      | ~23x faster                   |
| Dequeue   | 64         | ~30M/sec       | ~3x faster                    |
| Dequeue   | 1024       | ~45M/sec       | ~4x faster                    |

### Multi-Threaded Performance

| Configuration | Items/sec  | Notes                     |
|---------------|------------|---------------------------|
| 1p-1c (1024)  | ~234K/sec  | Baseline configuration    |
| 2p-2c (1024)  | ~200K/sec  | Slightly lower due to contention |
| 1p-4c (1024)  | ~133K/sec  | One consumer tends to process most items |

## Usage Example

```cpp
// Create a ring buffer with capacity 1024
RingBuffer<int, 1024> buffer;

// Producer thread
auto producer = [&buffer]() {
    for (int i = 0; i < 100; ++i) {
        while (!buffer.try_enqueue(i)) {
            // Optionally yield if buffer is full
            std::this_thread::yield();
        }
    }
};

// Consumer thread
auto consumer = [&buffer]() {
    int value;
    size_t items_processed = 0;
    
    while (items_processed < 100) {
        if (buffer.try_dequeue(value)) {
            // Process value
            items_processed++;
        } else {
            // Optionally yield if buffer is empty
            std::this_thread::yield();
        }
    }
};

// Launch threads
std::thread p(producer);
std::thread c(consumer);
p.join();
c.join();
```

## Limitations and Trade-offs

- **Fixed Capacity**: Buffer size must be known at compile time
- **Power-of-2 Restriction**: Capacity must be a power of 2 for optimal performance
- **Work Distribution**: In multi-consumer scenarios, work may not be evenly distributed
- **Memory Usage**: Pre-allocates the entire buffer capacity, which may be inefficient for large buffers with variable usage

## Future Improvements

- **Batch Operations**: Add support for enqueueing/dequeueing multiple items in a single operation
- **Backoff Strategy**: Implement exponential backoff for high-contention scenarios
- **Work Stealing**: Implement work stealing between consumer threads for better load balancing
- **Dynamic Sizing**: Explore options for runtime-resizable buffers while maintaining performance

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
./ring_buffer_bench
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.