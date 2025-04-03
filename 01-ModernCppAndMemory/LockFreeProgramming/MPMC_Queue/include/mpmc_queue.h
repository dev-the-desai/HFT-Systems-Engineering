/**
 * @file mpmc_queue.h
 * @brief Lock-Free Multi-Producer Multi-Consumer Queue Implementation
 * 
 * A high-performance, lock-free MPMC queue optimized for high-frequency trading applications.
 * This implementation focuses on fairness, throughput, and latency optimization.
 */

#pragma once

#include <atomic>
#include <array>
#include <cassert>
#include <optional>
#include <type_traits>
#include <cstddef>
#include <new>

// Alignment set once

// Ensure cache line alignment to prevent false sharing
constexpr size_t CACHE_LINE_SIZE = 64;

// Helper class for cache line padding
template<typename T>
struct alignas(CACHE_LINE_SIZE) CacheLineAligned {
    T data;
    
    CacheLineAligned() noexcept = default;
    explicit CacheLineAligned(const T& value) : data(value) {}
    explicit CacheLineAligned(T&& value) : data(std::move(value)) {}
    
    operator T&() noexcept { return data; }
    operator const T&() const noexcept { return data; }
    
    T& operator=(const T& value) noexcept {
        data = value;
        return data;
    }
    
    T& operator=(T&& value) noexcept {
        data = std::move(value);
        return data;
    }
};

// Alignment width set at instantiation
/**
 * @brief Aligns a value to the specified power of two
 * 
 * @tparam T The type to align
 * @tparam Alignment The alignment value (must be a power of two)
 */
/*
template <typename T, size_t Alignment>
struct alignas(Alignment) CacheAligned {
    T value;

    CacheAligned() noexcept = default;
    explicit CacheAligned(const T& v) noexcept : value(v) {}
    explicit CacheAligned(T&& v) noexcept : value(std::move(v)) {}

    operator T&() noexcept { return value; }
    operator const T&() const noexcept { return value; }
    
    T* operator&() noexcept { return &value; }
    const T* operator&() const noexcept { return &value; }
};
*/

/**
 * @brief Lock-free multi-producer multi-consumer queue
 * 
 * @tparam T The type of elements stored in the queue
 * @tparam Capacity The maximum number of elements the queue can hold (must be a power of two)
 * @tparam CacheLineSize The cache line size for alignment (default: 64 bytes)
 */
template <typename T, size_t Capacity, size_t CacheLineSize = 64>
class MPMCQueue {
    static_assert(Capacity > 0, "Capacity must be positive");
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
    static_assert(std::is_nothrow_copy_assignable_v<T> || std::is_nothrow_move_assignable_v<T>,
                  "T must be nothrow copy or move assignable");

public:
    /**
     * @brief Constructs an empty queue
     */
    MPMCQueue() noexcept : head_(0), tail_(0) {
        // Initialize all sequence counters
        for (size_t i = 0; i < Capacity; ++i) {
            slots_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    /**
     * @brief Non-copyable and non-movable
     */
    MPMCQueue(const MPMCQueue&) = delete;
    MPMCQueue& operator=(const MPMCQueue&) = delete;
    MPMCQueue(MPMCQueue&&) = delete;
    MPMCQueue& operator=(MPMCQueue&&) = delete;

    /**
     * @brief Attempts to enqueue an element
     * 
     * @param value The value to enqueue
     * @return true if the element was enqueued, false if the queue is full
     */
    template <typename U>
    bool enqueue(U&& value) noexcept {
        size_t head = head_.load(std::memory_order_relaxed);
        
        while (true) {
            // Get the slot at the current head position
            Slot& slot = slots_[head & mask_];
            size_t sequence = slot.sequence.load(std::memory_order_acquire);
            
            // Calculate the difference between sequence and head
            std::intptr_t diff = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(head);
            
            // If the slot is not ready for enqueue, the queue is full
            if (diff != 0) {
                // The slot is either not yet consumed or already enqueued
                if (diff < 0) {
                    // The queue is full
                    return false;
                }
                
                // Another thread has already moved the head, try again with the updated head
                head = head_.load(std::memory_order_relaxed);
                continue;
            }
            
            // Try to claim this slot by incrementing the head
            if (!head_.compare_exchange_weak(head, head + 1, 
                                            std::memory_order_relaxed)) {
                // Another thread claimed the slot, try again
                continue;
            }
            
            // Store the value in the claimed slot
            slot.element = std::forward<U>(value);
            
            // Mark the slot as ready for dequeue by setting the sequence to the next expected value
            slot.sequence.store(head + 1, std::memory_order_release);
            return true;
        }
    }

    /**
     * @brief Attempts to dequeue an element
     * 
     * @param result Reference to store the dequeued element
     * @return true if an element was dequeued, false if the queue is empty
     */
    bool dequeue(T& result) noexcept {
        size_t tail = tail_.load(std::memory_order_relaxed);
        
        while (true) {
            // Get the slot at the current tail position
            Slot& slot = slots_[tail & mask_];
            size_t sequence = slot.sequence.load(std::memory_order_acquire);
            
            // Calculate the difference between sequence and tail
            std::intptr_t diff = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(tail) - 1;
            
            // If the slot is not ready for dequeue, the queue is empty
            if (diff != 0) {
                // The slot is either not yet enqueued or already dequeued
                if (diff < 0) {
                    // The queue is empty
                    return false;
                }
                
                // Another thread has already moved the tail, try again with the updated tail
                tail = tail_.load(std::memory_order_relaxed);
                continue;
            }
            
            // Try to claim this slot by incrementing the tail
            if (!tail_.compare_exchange_weak(tail, tail + 1, 
                                            std::memory_order_relaxed)) {
                // Another thread claimed the slot, try again
                continue;
            }
            
            // Copy the value from the claimed slot
            result = std::move(slot.element);
            
            // Mark the slot as ready for enqueue by setting the sequence to the next expected value
            slot.sequence.store(tail + Capacity, std::memory_order_release);
            return true;
        }
    }

    /**
     * @brief Attempts to dequeue an element
     * 
     * @return std::optional<T> containing the element if successful, empty optional if queue is empty
     */
    std::optional<T> dequeue() noexcept {
        T result;
        if (dequeue(result)) {
            return std::optional<T>(std::move(result));
        }
        return std::nullopt;
    }

    /**
     * @brief Checks if the queue is empty
     * 
     * @note This is only a hint and may be inaccurate in a concurrent environment
     * @return true if the queue appears to be empty
     */
    bool empty() const noexcept {
        return head_.load(std::memory_order_relaxed) == 
               tail_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Returns the maximum capacity of the queue
     * 
     * @return The maximum number of elements the queue can hold
     */
    constexpr size_t capacity() const noexcept {
        return Capacity;
    }

    /**
     * @brief Estimates the current number of elements in the queue
     * 
     * @note This is only an estimate and may be inaccurate in a concurrent environment
     * @return The estimated number of elements
     */
    size_t size() const noexcept {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t tail = tail_.load(std::memory_order_relaxed);
        return head >= tail ? head - tail : 0;
    }

private:
    struct Slot {
        std::atomic<size_t> sequence;
        T element;
    };

    // Mask for fast modulo by capacity (works because capacity is a power of 2)
    static constexpr size_t mask_ = Capacity - 1;

    // Consumer counter
    alignas(CacheLineSize) std::atomic<size_t> tail_;
    
    // Producer counter
    alignas(CacheLineSize) std::atomic<size_t> head_;
    
    // Storage for elements and their sequence counters
    std::array<Slot, Capacity> slots_;
};
