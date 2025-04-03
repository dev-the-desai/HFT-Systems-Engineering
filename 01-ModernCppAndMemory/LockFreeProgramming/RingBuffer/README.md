# Lock-Free Ring Buffer

## Project Overview

This project implements a high-performance, lock-free circular buffer (ring buffer) designed for ultra-low-latency producer-consumer scenarios in trading systems. The ring buffer provides a fixed-size, pre-allocated memory region where multiple producers can write data and multiple consumers can read data without locks, maximizing throughput in latency-sensitive applications.

## Learning Objectives

- Implement a lock-free data structure using C++ atomic operations
- Apply memory ordering constraints correctly for thread safety
- Optimize for cache coherence and minimize false sharing
- Develop robust thread synchronization without locks
- Benchmark performance against traditional concurrent data structures

## Technical Requirements

- C++20 compiler (GCC 10+, Clang 10+, or MSVC 19.2+)
- CMake 3.16 or higher
- Google Benchmark library for performance testing
- Google Test for unit testing

## Implementation Plan

### Phase 1: Basic Single-Producer, Single-Consumer (SPSC) Ring Buffer

1. Create a fixed-size circular buffer with atomic head and tail pointers
2. Implement basic enqueue and dequeue operations
3. Ensure correct memory ordering with std::memory_order constraints
4. Add unit tests for correctness in single-threaded context

### Phase 2: Thread-Safety and Atomic Operations

1. Implement proper thread synchronization for SPSC scenario
2. Add cache line padding to prevent false sharing
3. Ensure lock-free property using only atomic operations
4. Add multi-threaded unit tests

### Phase 3: Multiple-Producer, Multiple-Consumer (MPMC) Extension

1. Extend the implementation to support multiple producers and consumers
2. Implement slot reservations using compare-and-swap operations
3. Add sequence counters to prevent ABA problems
4. Ensure correct operation under high contention

### Phase 4: Performance Optimization

1. Implement memory pre-faulting for predictable performance
2. Add optional busy-wait strategies for ultra-low latency
3. Optimize memory layout for cache efficiency
4. Add NUMA awareness for multi-socket systems

## Performance Benchmarks

The implementation will be benchmarked against:

1. std::queue with mutex
2. boost::lockfree::queue
3. folly::ProducerConsumerQueue
4. LMAX Disruptor (C++ port)

Metrics to measure:
- Throughput (operations/second)
- Latency (mean, 99th percentile, 99.9th percentile, max)
- CPU cache misses
- Context switches

## Advanced Features

- Zero-copy data transfer when possible
- Batched operations for higher throughput
- Configurable overflow policies (block, overwrite oldest)
- Optional timestamps for data items
- Support for variably-sized data with internal fragmentation handling

## HFT Application Examples

In the `examples` directory, we will provide sample use cases relevant to HFT:
1. Market data distribution to multiple strategy threads
2. Order management between strategy and execution components
3. Event processing pipeline with minimal latency

## References

- *C++ Concurrency in Action* (Anthony Williams)
- *The Art of Multiprocessor Programming* (Maurice Herlihy)
- *LMAX Disruptor Technical Paper*
- *Mechanical Sympathy* blog by Martin Thompson

## Timeline

- Week 1: Research and design
- Week 2: SPSC implementation and testing
- Week 3: MPMC extension and initial benchmarks
- Week 4: Optimization and advanced features

## Getting Started

```bash
mkdir build && cd build
cmake ..
make
./run_tests
./run_benchmarks
```