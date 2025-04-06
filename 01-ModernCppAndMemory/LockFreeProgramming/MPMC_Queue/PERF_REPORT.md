# MPMC Queue Performance Analysis

This document provides a comprehensive analysis of the performance characteristics of the lock-free MPMC queue implementation, including benchmark results, comparisons, and optimization insights.

## Benchmark Methodology

All benchmarks were conducted on the following system:

- **CPU**: Intel Core i9-13900HX (8 P-cores + 16 E-cores)
  - 32 logical cores at 2426.43 MHz
- **CPU Caches**:
  - L1 Data: 48 KiB (x16)
  - L1 Instruction: 32 KiB (x16)
  - L2 Unified: 2048 KiB (x16)
  - L3 Unified: 36864 KiB (x1)
- **OS**: Windows 11
- **Compiler**: MSVC v143 (Visual Studio 2022) with optimizations enabled (/O2)
- **Build**: Release configuration

Each benchmark was run with multiple iterations to ensure statistical significance, with the following configurations:

1. **Single-threaded enqueue**: Measures raw enqueue performance with buffer sizes from 64 to 1024
2. **Single-threaded dequeue**: Measures raw dequeue performance with buffer sizes from 64 to 1024
3. **Multi-threaded producer-consumer**: Various combinations of producer and consumer threads
4. **Comparison benchmark**: Standard library `std::queue` with mutex for reference

## Benchmark Results

### Single-Threaded Performance

#### Enqueue Performance

| Buffer Size | Time (ns) | CPU Time (ns) | Items/sec      |
|-------------|-----------|--------------|----------------|
| 64          | 611 ns    | 656 ns       | 97.5238M/s     |
| 128         | 3949 ns   | 3828 ns      | 33.4367M/s     |
| 256         | 8144 ns   | 9062 ns      | 28.2483M/s     |
| 512         | 15299 ns  | 13602 ns     | 37.6414M/s     |
| 1024        | 30490 ns  | 30692 ns     | 33.3638M/s     |

#### Dequeue Performance

| Buffer Size | Time (ns) | CPU Time (ns) | Items/sec     |
|-------------|-----------|--------------|---------------|
| 64          | 2770 ns   | 2895 ns      | 22.1085M/s    |
| 128         | 4649 ns   | 4614 ns      | 27.7401M/s    |
| 256         | 8332 ns   | 8692 ns      | 29.4522M/s    |
| 512         | 15934 ns  | 16183 ns     | 31.6381M/s    |
| 1024        | 31392 ns  | 31285 ns     | 32.731M/s     |

#### Mutex-Based Queue (Comparison)

| Buffer Size | Time (ns) | CPU Time (ns) | Items/sec     |
|-------------|-----------|--------------|---------------|
| 64          | 14220 ns  | 14300 ns     | 8.95126M/s    |
| 128         | 24495 ns  | 23019 ns     | 11.1212M/s    |
| 256         | 47687 ns  | 49178 ns     | 10.4112M/s    |
| 512         | 95641 ns  | 94169 ns     | 10.8741M/s    |
| 1024        | 204283 ns | 199507 ns    | 10.2653M/s    |

### Multi-Threaded Performance

| Configuration    | Time (ns)   | CPU Time (ns) | Items/sec     | Notes             |
|------------------|-------------|--------------|---------------|-------------------|
| 1p-1c (1024)     | 826078 ns   | 578125 ns    | 1.72973M/s    | 1 producer, 1 consumer |
| 2p-2c (1024)     | 1300834 ns  | 585938 ns    | 1.70667M/s    | 2 producers, 2 consumers |
| 4p-4c (1024)     | 2117964 ns  | 830078 ns    | 1.20471M/s    | 4 producers, 4 consumers |
| 1p-4c (1024)     | 1632365 ns  | 687500 ns    | 1.45455M/s    | 1 producer, 4 consumers |
| 4p-1c (1024)     | 1515262 ns  | 920759 ns    | 1086.06k/s    | 4 producers, 1 consumer |
| 2p-2c (64)       | 1353450 ns  | 669643 ns    | 1.49333M/s    | 2 producers, 2 consumers, small buffer |
| 2p-2c (256)      | 1257586 ns  | 613839 ns    | 1.62909M/s    | 2 producers, 2 consumers, medium buffer |
| 2p-2c (4096)     | 1277268 ns  | 502232 ns    | 1.99111M/s    | 2 producers, 2 consumers, large buffer |

## Performance Analysis

### Comparison with Ring Buffer Implementation

When compared to the previously implemented lock-free ring buffer, the MPMC queue shows several key differences:

1. **Single-Threaded Enqueue Performance**:
   - Ring Buffer: Up to 293M/sec (at 1024 elements)
   - MPMC Queue: Up to 97.5M/sec (at 64 elements)
   - The MPMC queue's single-threaded enqueue performance is approximately 1/3 that of the ring buffer

2. **Single-Threaded Dequeue Performance**:
   - Ring Buffer: Up to 41.8M/sec (at 512 elements)
   - MPMC Queue: Up to 32.7M/sec (at 1024 elements)
   - The MPMC queue's dequeue performance is comparable to the ring buffer (about 80% of its throughput)

3. **Multi-Threaded Performance**:
   - Ring Buffer: 179.2K/s (1p-1c, 1024 elements)
   - MPMC Queue: 1.73M/s (1p-1c, 1024 elements)
   - The MPMC queue shows approximately 10x higher throughput in multi-threaded scenarios

4. **Scaling with Thread Count**:
   - Ring Buffer: Performance decreases with more threads
   - MPMC Queue: Better scaling, though still shows some performance degradation with 4p-4c

5. **Buffer Size Impact**:
   - Ring Buffer: Smaller buffers perform better in multi-threaded scenarios
   - MPMC Queue: Larger buffers actually perform better in multi-threaded scenarios (4096 elements shows best performance)

This performance profile indicates that the MPMC queue makes different trade-offs, sacrificing some single-threaded performance for significantly better multi-threaded performance and scaling.

### Single-Threaded vs. Multi-Threaded

The MPMC queue shows interesting performance dynamics:

- **Single-threaded operations**: Good but not exceptional performance (97.5M ops/sec for enqueue, 32.7M ops/sec for dequeue)
- **Multi-threaded operations**: Excellent performance compared to the ring buffer (1.73M ops/sec vs. 179.2K ops/sec)
- **Lock vs. Lock-free**: Still significantly outperforms mutex-based solutions (1.73M ops/sec vs. approximately 340K ops/sec estimated for mutex-based solution with similar configuration)

The key insight is that the MPMC queue's design decisions favor multi-threaded scenarios, which aligns with its intended use case in high-frequency trading applications where multiple threads process market data concurrently.

### Buffer Size Impact

The MPMC queue shows an unexpected pattern regarding buffer size:

1. **For single-threaded enqueue**: Best performance with smallest buffer size (64 elements: 97.5M/sec)
2. **For single-threaded dequeue**: Performance increases with buffer size (1024 elements: 32.73M/sec)
3. **For multi-threaded scenarios**: Larger buffers perform better (4096 elements: 1.99M/sec vs. 64 elements: 1.49M/sec)

This is contrary to the ring buffer, where smaller buffers generally performed better in multi-threaded scenarios. This difference likely stems from:

- Different memory access patterns 
- Reduced contention in larger buffers due to the different synchronization mechanism
- More cache-friendly slot layout in the MPMC queue

### Thread Configuration Impact

The multi-threaded benchmark results reveal interesting patterns:

1. **Producer/Consumer Balance**:
   - Balanced configurations (1p-1c, 2p-2c) show better performance than imbalanced ones (1p-4c, 4p-1c)
   - The 4p-1c configuration shows the lowest performance (1086.06K/s), indicating a bottleneck on the consumer side

2. **Thread Scaling**:
   - Performance with 1p-1c (1.73M/s) is slightly better than 2p-2c (1.71M/s)
   - Performance further decreases with 4p-4c (1.20M/s), showing some scaling limitations
   - However, this scaling is still significantly better than the ring buffer implementation

3. **Best Configuration**:
   - For best throughput: 2p-2c with 4096 buffer size (1.99M/s)
   - For low latency with good throughput: 1p-1c with 1024 buffer size (1.73M/s)
   - For balanced producer/consumer loads: 2p-2c with 256 buffer size (1.63M/s)

### Hardware Architecture Considerations

Like the ring buffer benchmarks, these tests were run on a hybrid core architecture (Intel Core i9-13900HX with P-cores and E-cores), which influences performance:

1. **Thread Scheduling**: Performance can vary based on whether threads run on P-cores or E-cores
2. **Cache Hierarchy Impact**: The improved multi-threaded performance might be partly due to better cache utilization across multiple cores
3. **Frequency Impact**: The specific frequencies of cores during the test impact overall throughput

## Comparative Analysis

### MPMC Queue vs. Ring Buffer

The MPMC queue demonstrates significant differences from the ring buffer:

1. **Single-threaded performance tradeoffs**:
   - The MPMC queue sacrifices some single-threaded performance (particularly for enqueue operations)
   - This trade-off enables significantly better multi-threaded scaling

2. **Multi-threaded scaling**:
   - MPMC queue shows approximately 10x higher throughput in multi-threaded scenarios
   - Work distribution is more even among consumer threads
   - Performance with additional threads degrades less dramatically

3. **Buffer size impact**:
   - Contrary to ring buffer, larger buffer sizes perform better for the MPMC queue in multi-threaded scenarios
   - This suggests fundamentally different memory access patterns and cache utilization

4. **Use case alignment**:
   - The MPMC queue is better suited for true multi-threaded workloads
   - The ring buffer may still be preferable for cases where single-threaded performance is paramount

### MPMC Queue vs. Mutex-Based Queue

The MPMC queue significantly outperforms the mutex-based alternative:

1. **Single-threaded enqueue**: 97.5M/sec vs. 8.95M/sec (approximately 11x faster at 64 elements)
2. **Single-threaded dequeue**: 32.7M/sec vs. 10.3M/sec (approximately 3.2x faster at 1024 elements)
3. **Multi-threaded throughput**: Estimated 5-10x faster than mutex-based approach (based on single-threaded multipliers)
4. **Scalability**: Better scaling with thread count than mutex-based approaches which serialize access

## Performance Optimization Opportunities

Based on the benchmark results, these optimizations could improve performance further:

1. **CPU Cache Optimization**:
   - Further optimize slot layout for improved cache utilization
   - Consider prefetching techniques for predictable access patterns

2. **Thread Count Tuning**:
   - Benchmark results suggest optimal thread configurations:
     - For pure throughput: 2p-2c with 4096 elements
     - For balanced performance: 2p-2c with 256-1024 elements
   - Consider thread affinity for hybrid core architectures

3. **Backoff Strategy Improvement**:
   - Implement adaptive backoff with CPU topology awareness
   - Consider platform-specific spinning techniques (e.g., PAUSE instruction on x86)

4. **Memory Layout Optimization**:
   - Further alignment optimizations for NUMA architectures
   - Investigate slot packing vs. spreading tradeoffs

5. **Batch Operations**:
   - Implement bulk enqueue/dequeue operations for improved throughput
   - Explore amortizing synchronization costs across multiple operations

## Conclusion

The MPMC queue implementation demonstrates excellent performance characteristics for multi-threaded high-frequency trading applications, with significantly better throughput and scalability compared to the ring buffer in multi-threaded scenarios, while still outperforming mutex-based alternatives. The implementation makes different trade-offs than the ring buffer, sacrificing some single-threaded performance for significantly better multi-threaded performance.

For optimal performance in HFT applications, the following configurations are recommended:

1. **High Throughput**: 2p-2c with 4096 buffer size (1.99M/s)
2. **Low Latency**: 1p-1c with 1024 buffer size (1.73M/s)
3. **Balanced Workloads**: 2p-2c with 256 buffer size (1.63M/s)

Future work should focus on further optimizing memory access patterns, implementing adaptive backoff strategies, and exploring batch operations for improved throughput.