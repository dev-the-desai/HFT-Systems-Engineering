#include "../include/ring_buffer.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>


#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif


// Basic functionality tests
TEST(RingBufferTest, BasicOperations) {
    RingBuffer<int, 16> buffer;
    
    // Test empty checks
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(buffer.size(), 0);
    
    // Test enqueue
    EXPECT_TRUE(buffer.try_enqueue(42));
    EXPECT_FALSE(buffer.empty());
    EXPECT_EQ(buffer.size(), 1);
    
    // Test dequeue
    int value;
    EXPECT_TRUE(buffer.try_dequeue(value));
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(buffer.empty());
    
    // Test optional dequeue
    buffer.try_enqueue(100);
    auto result = buffer.try_dequeue();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 100);
    EXPECT_TRUE(buffer.empty());
    
    // Test dequeue from empty buffer
    EXPECT_FALSE(buffer.try_dequeue(value));
    result = buffer.try_dequeue();
    EXPECT_FALSE(result.has_value());
}

// Test filling the buffer to capacity
TEST(RingBufferTest, FillingToCapacity) {
    constexpr size_t CAPACITY = 8;
    RingBuffer<int, CAPACITY> buffer;
    
    // Fill the buffer
    for (size_t i = 0; i < CAPACITY; ++i) {
        EXPECT_TRUE(buffer.try_enqueue(static_cast<int>(i)));
    }
    
    EXPECT_TRUE(buffer.full());
    EXPECT_EQ(buffer.size(), CAPACITY);
    
    // Try adding one more (should fail)
    EXPECT_FALSE(buffer.try_enqueue(100));
    
    // Remove one item
    int value;
    EXPECT_TRUE(buffer.try_dequeue(value));
    EXPECT_EQ(value, 0);
    EXPECT_FALSE(buffer.full());
    
    // Now we should be able to add one
    EXPECT_TRUE(buffer.try_enqueue(100));
    
    // Empty the buffer and check values
    for (size_t i = 1; i < CAPACITY; ++i) {
        EXPECT_TRUE(buffer.try_dequeue(value));
        EXPECT_EQ(value, static_cast<int>(i));
    }
    
    // Last item should be the one we added after removing one
    EXPECT_TRUE(buffer.try_dequeue(value));
    EXPECT_EQ(value, 100);
    EXPECT_TRUE(buffer.empty());
}

// Test wraparound behavior
TEST(RingBufferTest, Wraparound) {
    constexpr size_t CAPACITY = 4;
    RingBuffer<int, CAPACITY> buffer;
    int value;
    
    // Fill and drain the buffer multiple times to test wraparound
    for (int iteration = 0; iteration < 10; ++iteration) {
        // Fill
        for (size_t i = 0; i < CAPACITY; ++i) {
            EXPECT_TRUE(buffer.try_enqueue(static_cast<int>(i + iteration * 100)));
        }
        
        // Drain
        for (size_t i = 0; i < CAPACITY; ++i) {
            EXPECT_TRUE(buffer.try_dequeue(value));
            EXPECT_EQ(value, static_cast<int>(i + iteration * 100));
        }
    }
    
    EXPECT_TRUE(buffer.empty());
}

// Test with a more complex data type
struct TestObject {
    int id;
    std::string name;
    
    TestObject() : id(0), name("default") {}
    TestObject(int i, std::string n) : id(i), name(std::move(n)) {}
    
    // Make objects comparable for testing
    bool operator==(const TestObject& other) const {
        return id == other.id && name == other.name;
    }
};

TEST(RingBufferTest, ComplexDataType) {
    RingBuffer<TestObject, 4> buffer;
    
    // Add objects
    TestObject obj1(1, "one");
    TestObject obj2(2, "two");
    
    EXPECT_TRUE(buffer.try_enqueue(obj1));
    EXPECT_TRUE(buffer.try_enqueue(obj2));
    EXPECT_EQ(buffer.size(), 2);
    
    // Retrieve objects
    TestObject result;
    EXPECT_TRUE(buffer.try_dequeue(result));
    EXPECT_EQ(result, obj1);
    
    EXPECT_TRUE(buffer.try_dequeue(result));
    EXPECT_EQ(result, obj2);
    
    EXPECT_TRUE(buffer.empty());
}

// Test with move-only types
class MoveOnlyType {
public:
    MoveOnlyType(int val) : value_(val) {}
    
    // Move constructor
    MoveOnlyType(MoveOnlyType&& other) noexcept : value_(other.value_) {
        other.value_ = 0;
    }
    
    // Move assignment
    MoveOnlyType& operator=(MoveOnlyType&& other) noexcept {
        if (this != &other) {
            value_ = other.value_;
            other.value_ = 0;
        }
        return *this;
    }
    
    // Delete copy operations
    MoveOnlyType(const MoveOnlyType&) = delete;
    MoveOnlyType& operator=(const MoveOnlyType&) = delete;
    
    // Default constructor for the array
    MoveOnlyType() : value_(0) {}
    
    int getValue() const { return value_; }
    
private:
    int value_;
};

TEST(RingBufferTest, MoveOnlyType) {
    RingBuffer<MoveOnlyType, 4> buffer;
    
    EXPECT_TRUE(buffer.try_enqueue(MoveOnlyType(42)));
    EXPECT_TRUE(buffer.try_enqueue(MoveOnlyType(43)));
    
    auto result = buffer.try_dequeue();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->getValue(), 42);
    
    MoveOnlyType value;
    EXPECT_TRUE(buffer.try_dequeue(value));
    EXPECT_EQ(value.getValue(), 43);
}

/*
// Multi-threaded tests
TEST(RingBufferTest, MultiThreaded) {
    // Lighter Test
    constexpr size_t CAPACITY = 1024;
    constexpr size_t NUM_ITEMS = 100;
    constexpr size_t NUM_PRODUCERS = 2;
    constexpr size_t NUM_CONSUMERS = 2;

    // Heavier Test
    constexpr size_t CAPACITY = 128;
    constexpr size_t NUM_ITEMS = 10000;
    constexpr size_t NUM_PRODUCERS = 4;
    constexpr size_t NUM_CONSUMERS = 4;

    RingBuffer<int, CAPACITY> buffer;
    std::atomic<size_t> total_consumed(0);
    std::atomic<bool> done(false);
    
    // Producer function
    auto producer_func = [&](int producer_id) {
        for (size_t i = 0; i < NUM_ITEMS; ++i) {
            int item = static_cast<int>(producer_id * NUM_ITEMS + i);
            while (!buffer.try_enqueue(item)) {
                std::this_thread::yield();
            }
        }
    };
  
    // Consumer function
    auto consumer_func = [&]() {
        while (total_consumed.load() < NUM_ITEMS * NUM_PRODUCERS && !done.load()) {
            auto result = buffer.try_dequeue();
            if (result.has_value()) {
                total_consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    };
    
    
    // Create and start producer threads
    std::vector<std::thread> producers;
    for (size_t i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(producer_func, i);
    }
    
    // Create and start consumer threads
    std::vector<std::thread> consumers;
    for (size_t i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consumer_func);
    }
    
    // Join producer threads
    for (auto& t : producers) {
        t.join();
    }
    
    // Wait for all items to be consumed or timeout
    auto start = std::chrono::steady_clock::now();
    while (total_consumed.load() < NUM_ITEMS * NUM_PRODUCERS) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if (elapsed > 5) {  // 15 second timeout
            done.store(true);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Signal consumers to finish and join them
    done.store(true);
    for (auto& t : consumers) {
        t.join();
    }
    
    // Verify all items were consumed
    EXPECT_EQ(total_consumed.load(), NUM_ITEMS * NUM_PRODUCERS);
    EXPECT_TRUE(buffer.empty());
}
*/


void set_thread_affinity(std::thread& t, unsigned core_id) {
    #ifdef _WIN32
        SetThreadAffinityMask(t.native_handle(), (1ULL << core_id));
    #else
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);
        pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    #endif
}


TEST(RingBufferTest, MultiThreaded) {
    // Test parameters
    constexpr size_t NUM_PRODUCERS = 2;
    constexpr size_t NUM_CONSUMERS = 2;
    constexpr size_t NUM_ITEMS = 100;
    constexpr size_t BUFFER_SIZE = 1024;

    // Create the ring buffer
    RingBuffer<int, BUFFER_SIZE> buffer;

    // Atomic synchronization variables
    std::atomic<size_t> total_produced(0);
    std::atomic<size_t> total_consumed(0);
    std::atomic<bool> done(false);

    // Produce items
    auto producer_func = [&]() {
        for (size_t i = 0; i < NUM_ITEMS; ++i) {
            int value = i;
            while (!buffer.try_enqueue(value)) {
                if (done.load(std::memory_order_relaxed)) {
                    return;
                }
                std::this_thread::yield();
            }
            total_produced.fetch_add(1, std::memory_order_relaxed);
        }
    };

    // Consume items
    auto consumer_func = [&]() {
        std::vector<int> items_seen;
        
        while (total_consumed.load(std::memory_order_relaxed) < NUM_ITEMS * NUM_PRODUCERS) {
            int value;
            if (buffer.try_dequeue(value)) {
                items_seen.push_back(value);
                total_consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                // Prevent tight spinning
                std::this_thread::yield();
            }

            // Early termination safety
            if (done.load(std::memory_order_relaxed)) {
                break;
            }
        }

        // Debug logging
        std::cout << "Consumer finished, saw " << items_seen.size() << " items\n";
    };

    // Create producer threads
    std::vector<std::thread> producers;
    for (size_t i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(producer_func);
        set_thread_affinity(producers.back(), i % 8); // No impact setting affinity
    }

    // Create consumer threads
    std::vector<std::thread> consumers;
    for (size_t i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consumer_func);
        set_thread_affinity(consumers.back(), (i + NUM_PRODUCERS) % 8);   // No impact setting affinity
    }

    // Wait for all threads to complete
    for (auto& producer : producers) {
        producer.join();
    }

    // Allow some time for consumers to process remaining items
    auto start_time = std::chrono::steady_clock::now();
    while (total_consumed.load(std::memory_order_relaxed) < total_produced.load(std::memory_order_relaxed)) {
        std::this_thread::yield();
        
        // Prevent infinite wait
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count() > 5) {
            done.store(true, std::memory_order_relaxed);
            break;
        }
    }

    // Mark as done to help threads exit
    done.store(true, std::memory_order_relaxed);

    // Join consumer threads
    for (auto& consumer : consumers) {
        consumer.join();
    }

    // Final assertions with more detailed diagnostics
    EXPECT_EQ(total_produced.load(), NUM_ITEMS * NUM_PRODUCERS) 
        << "Not all items were produced. Produced: " << total_produced.load();
    
    EXPECT_EQ(total_consumed.load(), total_produced.load()) 
        << "Consumed items (" << total_consumed.load() 
        << ") do not match produced items (" << total_produced.load() << ")";
    
    EXPECT_TRUE(buffer.empty()) 
        << "Buffer should be empty after processing all items";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
