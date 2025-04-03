# MPMC Queue Implementation Details

This document explains the design decisions and implementation details of the lock-free Multiple-Producer Multiple-Consumer (MPMC) queue.

## Core Design

The MPMC queue implementation is based on a bounded array of slots, where each slot consists of:
1. An atomic sequence counter
2. Storage for the element

The key insight is using sequence counters to coordinate access between producers and consumers without locks. The queue operates by using the following indexes:

- `head`: The next position to be written (producer index)
- `tail`: The next position to be read (consumer index)

Both indexes are cache-line aligned to prevent false sharing.

## Lock-Free Algorithm

### Enqueue Operation

1. A producer reads the current `head` value atomically
2. It checks the sequence counter of the slot at `head % capacity`
3. If the sequence equals `head`, the slot is available for enqueue
4. The producer attempts to increment `head` using compare-and-swap (CAS)
5. If CAS succeeds, the producer owns the slot and writes the element
6. The producer updates the slot's sequence to `head + 1` to signal it contains valid data

### Dequeue Operation

1. A consumer reads the current `tail` value atomically
2. It checks the sequence counter of the slot at `tail % capacity`
3. If the sequence equals `tail + 1`, the slot contains valid data ready for dequeue
4. The consumer attempts to increment `tail` using compare-and-swap (CAS)
5. If CAS succeeds, the consumer owns the slot and reads the element
6. The consumer updates the slot's sequence to `tail + capacity` to signal it's available for producers

## Memory Ordering

The implementation uses carefully chosen memory ordering constraints to ensure correct behavior while minimizing overhead:

- `std::memory_order_relaxed` for most operations on the head and tail counters
- `std::memory_order_acquire` when reading sequences to see updates from other threads
- `std::memory_order_release` when writing sequences to make updates visible to other threads

## Optimization Details

### Cache Alignment

Both the head and tail counters are aligned to cache line boundaries (64 bytes on most architectures) to prevent false sharing. This alignment ensures that when one thread updates the head counter, it doesn't invalidate the cache line containing the tail counter that might be accessed by another thread.

```cpp
// Consumer counter
alignas(CacheLineSize) std::atomic<size_t> tail_;

// Producer counter
alignas(CacheLineSize) std::atomic<size_t> head_;
```

### Power-of-Two Capacity

The queue capacity is constrained to be a power of two. This allows for efficient calculation of the array index using a bitmask:

```cpp
// Mask for fast modulo by capacity (works because capacity is a power of 2)
static constexpr size_t mask_ = Capacity - 1;

// Fast index calculation
Slot& slot = slots_[head & mask_];
```

This approach avoids expensive modulo operations that would otherwise be required.

### Contention Handling

The implementation uses a technique called exponential backoff to handle contention. When a thread fails to claim a slot, it might spin for a short time before retrying, which can improve performance under heavy contention by reducing cache coherence traffic.

## Thread Safety and Correctness

The algorithm ensures that:

1. Each slot is only written to by one producer at a time
2. Each slot is only read by one consumer at a time
3. A consumer never reads from a slot that hasn't been fully written by a producer
4. A producer never overwrites a slot that hasn't been fully read by a consumer

These guarantees are maintained through careful management of the sequence counters and atomic operations, without using traditional locks.

## Performance Characteristics

The lock-free MPMC queue is designed to offer:

- **Throughput**: High number of operations per second, especially in multi-threaded scenarios
- **Fairness**: All producers and consumers get fair access to the queue
- **Low Latency**: Individual operations complete quickly, with predictable performance
- **Scalability**: Performance scales well with the number of threads, up to hardware limits

## Limitations

1. **Bounded Capacity**: The queue has a fixed capacity that must be specified at compile time
2. **Power-of-Two Constraint**: The capacity must be a power of two
3. **ABA Problem Handling**: The sequence counter approach addresses the ABA problem, but adds some complexity
4. **No Blocking Operations**: The queue operations are non-blocking, so consumers must poll or use other synchronization mechanisms if they need to wait for data

## Comparison with Other Approaches

Compared to mutex-based queues, this implementation offers:
- Better scaling with multiple threads
- Lower latency for individual operations
- No risk of priority inversion or lock contention

Compared to other lock-free queues:
- This design focuses on throughput and fairness
- The algorithm is optimized for modern CPU architectures with efficient cache coherency protocols
- The implementation is relatively simple while maintaining correctness
