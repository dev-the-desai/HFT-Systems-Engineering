#pragma once

#include <atomic>
#include <cstddef>
#include <array>
#include <optional>
#include <type_traits>

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

/**
 * @brief A lock-free ring buffer implementation optimized for high-performance trading applications
 * 
 * This ring buffer provides a fixed-size, pre-allocated memory region for producer-consumer
 * communication without locks. The implementation ensures thread safety using atomic operations
 * and memory ordering constraints.
 * 
 * @tparam T The type of elements stored in the buffer
 * @tparam Capacity The fixed capacity of the buffer (must be a power of 2)
 */
template<typename T, size_t Capacity>
class RingBuffer {
    static_assert(Capacity > 0, "Capacity must be greater than 0");
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");
    static_assert(std::is_move_constructible_v<T>, "T must be move constructible");

public: 
    /**
     * @brief Constructs a new Ring Buffer with the specified capacity
     */
    RingBuffer() noexcept {
        // Initialize atomic counters
        head_.data.store(0, std::memory_order_relaxed);
        tail_.data.store(0, std::memory_order_relaxed);

        // Pre-fault the memory to avoid page faults during operation
        for (size_t i = 0; i < Capacity; ++i) {
            new (&buffer_[i]) T();
        }
    }

    /**
     * @brief Destroys the Ring Buffer and its contents
     */
    ~RingBuffer() {
        // Destroy all elements still in the buffer
        size_t head = head_.data.load(std::memory_order_relaxed);
        size_t tail = tail_.data.load(std::memory_order_relaxed);
        
        while (head != tail) {
            buffer_[head & mask_].~T();
            head++;
        }
    }

    // Disable copying to avoid concurrent access issues
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    /**
     * @brief Attempts to enqueue an element to the buffer
     * 
     * @param item The item to enqueue
     * @return true if successful, false if the buffer is full
     */
    bool try_enqueue(const T& item) noexcept {
        size_t head = head_.data.load(std::memory_order_relaxed);
        size_t next_head = head + 1;
        size_t tail = tail_.data.load(std::memory_order_acquire);
        
        // Check if buffer is full
        if (next_head - tail > Capacity) {
            return false;
        }
        
        // Place the item in the buffer
        buffer_[head & mask_] = item;
        
        // Update the head pointer with a release operation to ensure visibility
        head_.data.store(next_head, std::memory_order_release);
        return true;
    }

    /**
     * @brief Attempts to enqueue an element using move semantics
     * 
     * @param item The item to move into the buffer
     * @return true if successful, false if the buffer is full
     */
    bool try_enqueue(T&& item) noexcept {
        size_t head = head_.data.load(std::memory_order_relaxed);
        size_t next_head = head + 1;
        size_t tail = tail_.data.load(std::memory_order_acquire);
        
        // Check if buffer is full
        if (next_head - tail > Capacity) {
            return false;
        }
        
        // Move the item into the buffer
        buffer_[head & mask_] = std::move(item);
        
        // Update the head pointer with a release operation
        head_.data.store(next_head, std::memory_order_release);
        return true;
    }

    /**
     * @brief Attempts to dequeue an element from the buffer
     * 
     * @param[out] result Reference to store the dequeued item
     * @return true if successful, false if the buffer is empty
     */
    bool try_dequeue(T& result) noexcept {
        size_t tail = tail_.data.load(std::memory_order_relaxed);
        size_t head = head_.data.load(std::memory_order_acquire);
        
        // Check if buffer is empty
        if (head <= tail) {
            return false;
        }
        
        // Get the item from the buffer
        result = std::move(buffer_[tail & mask_]);

        // Try to atomically update the tail pointer
        if (tail_.data.compare_exchange_strong(tail, tail + 1, 
                std::memory_order_release, 
                std::memory_order_relaxed)) {
            return true;  // Successfully dequeued
        }

        return false;
    }

    /**
     * @brief Attempts to dequeue an element from the buffer
     * 
     * @return std::optional<T> containing the dequeued item, or std::nullopt if empty
     */
    std::optional<T> try_dequeue() noexcept {
        size_t tail = tail_.data.load(std::memory_order_relaxed);
        size_t head = head_.data.load(std::memory_order_acquire);
        
        // Check if buffer is empty
        if (head <= tail) {
            return std::nullopt;
        }
        
        // Get the item from the buffer
        T result = std::move(buffer_[tail & mask_]);
        
        // Try to atomically update the tail pointer
        if (tail_.data.compare_exchange_strong(tail, tail + 1, 
                std::memory_order_release, 
                std::memory_order_relaxed)) {
            return std::optional<T>(std::move(result));  // Successfully dequeued
        }

        // If compare_exchange fails, return empty result
        return std::nullopt;
    }

    /**
     * @brief Returns the current number of elements in the buffer
     * 
     * Note: This is a snapshot and may change if other threads are concurrently
     * accessing the buffer.
     * 
     * @return size_t The number of elements currently in the buffer
     */
    size_t size() const noexcept {
        // Use acquire-acquire for consistent view
        size_t head = head_.data.load(std::memory_order_acquire);
        size_t tail = tail_.data.load(std::memory_order_acquire);
        return head - tail;
    }

    /**
     * @brief Checks if the buffer is empty
     * 
     * @return true if empty, false otherwise
     */
    bool empty() const noexcept {
        return size() == 0;
    }

    /**
     * @brief Checks if the buffer is full
     * 
     * @return true if full, false otherwise
     */
    bool full() const noexcept {
        return size() >= Capacity;
    }

    /**
     * @brief Returns the capacity of the buffer
     * 
     * @return constexpr size_t The buffer capacity
     */
    constexpr size_t capacity() const noexcept {
        return Capacity;
    }

private:
    // Mask for fast modulo calculation (works because Capacity is power of 2)
    static constexpr size_t mask_ = Capacity - 1;
    
    // Head and tail pointers, aligned to cache lines to prevent false sharing
    CacheLineAligned<std::atomic<size_t>> head_;
    CacheLineAligned<std::atomic<size_t>> tail_;
    
    // Storage for elements
    std::array<T, Capacity> buffer_;
};
