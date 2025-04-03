# Modern C++ and Memory Management Reading List

## Core C++ References

1. **Effective Modern C++ (Scott Meyers)**
   - Focus on: Move semantics, perfect forwarding, lambda expressions
   - Chapters 5-7 are particularly relevant for performance

2. **C++ Concurrency in Action, 2nd Edition (Anthony Williams)**
   - Focus on: Memory model, atomics, lock-free programming
   - Chapters 5, 7, and 8 are essential for HFT

3. **The C++ Programming Language, 4th Edition (Bjarne Stroustrup)**
   - Reference for C++11/14 language features
   - Key sections: Templates, Move Semantics, Concurrency

## Performance-Focused Resources

4. **CPU Caches and Why You Care (Scott Meyers)**
   - Video: [https://www.youtube.com/watch?v=WDIkqP4JbkE](https://www.youtube.com/watch?v=WDIkqP4JbkE)
   - Essential for understanding memory access patterns

5. **What Every Programmer Should Know About Memory (Ulrich Drepper)**
   - Paper: [https://people.freebsd.org/~lstewart/articles/cpumemory.pdf](https://people.freebsd.org/~lstewart/articles/cpumemory.pdf)
   - Comprehensive overview of memory hierarchy and optimization

6. **Writing Quick Code in C++, Quickly (Andrei Alexandrescu)**
   - CppCon talk: [https://www.youtube.com/watch?v=ea5DiCg8HOY](https://www.youtube.com/watch?v=ea5DiCg8HOY)
   - Practical techniques for high-performance C++

## Lock-Free Programming

7. **Lock-Free Programming (Herb Sutter)**
   - CppCon talks: Parts 1 & 2
   - Foundational concepts and patterns

8. **The Art of Multiprocessor Programming (Maurice Herlihy & Nir Shavit)**
   - Chapters on lock-free and wait-free algorithms
   - Theoretical foundation with practical applications

9. **An Introduction to Lock-Free Programming (Preshing on Programming)**
   - Blog series: [https://preshing.com/20120612/an-introduction-to-lock-free-programming/](https://preshing.com/20120612/an-introduction-to-lock-free-programming/)
   - Accessible explanation of core concepts

## Memory Management

10. **A Custom Memory Allocator in C++ (Bob Steagall)**
    - CppCon talk: [https://www.youtube.com/watch?v=kSWfushlvB8](https://www.youtube.com/watch?v=kSWfushlvB8)
    - Practical implementation of a custom allocator

11. **Memory Allocators 101 (Tomasz Kapela)**
    - Comprehensive guide to memory allocator design
    - Implementation strategies for different use cases

12. **The Mallocator (Andrei Alexandrescu)**
    - Design patterns for efficient allocation
    - Advanced techniques for specialized allocators

## HFT-Specific Resources

13. **Optimizing Software in C++ (Agner Fog)**
    - Guide: [https://www.agner.org/optimize/optimizing_cpp.pdf](https://www.agner.org/optimize/optimizing_cpp.pdf)
    - Low-level optimization techniques relevant to HFT

14. **Mechanical Sympathy (Martin Thompson)**
    - Blog: [https://mechanical-sympathy.blogspot.com/](https://mechanical-sympathy.blogspot.com/)
    - Articles on hardware-conscious programming

15. **LMAX Disruptor: High-performance inter-thread messaging library**
    - Technical paper: [https://lmax-exchange.github.io/disruptor/files/Disruptor-1.0.pdf](https://lmax-exchange.github.io/disruptor/files/Disruptor-1.0.pdf)
    - Example of high-performance message passing used in trading

## C++20 Features Relevant to Performance

16. **C++20 Features: A Quick Overview (Marc Gregoire)**
   - Focus on: coroutines, atomic<shared_ptr>, atomic_wait

17. **C++ Core Guidelines on Performance (Bjarne Stroustrup & Herb Sutter)**
   - Performance section: [https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#per-performance](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#per-performance)

## Research Papers

18. **Access Pattern-Aware Cache Management for Improving Data Utilization in GPU (2023)**
    - Relevant for optimizing memory access patterns

19. **Custom Memory Allocation in High-Performance Trading Systems (Citadel Securities)**
    - Overview of allocation strategies in production systems

20. **Zero-Copy Serialization for Low-Latency Applications (Jane Street, 2021)**
    - Techniques applicable to market data processing

## Applied Learning Resources

- **Compiler Explorer**: [https://godbolt.org/](https://godbolt.org/) - For examining compiler optimizations
- **quick-bench**: [https://quick-bench.com/](https://quick-bench.com/) - For benchmarking code snippets
- **Valgrind & Cachegrind**: For memory and cache profiling

## Reading Order Recommendation

For optimal progression through this material:

1. Start with C++ fundamentals (Items 1-3)
2. Move to cache and memory architecture (Items 4-5)
3. Study lock-free programming concepts (Items 7-9)
4. Explore memory allocator design (Items 10-12)
5. Apply knowledge to HFT-specific material (Items 13-15)
6. Research phase: study papers and advanced topics (Items 16-20)