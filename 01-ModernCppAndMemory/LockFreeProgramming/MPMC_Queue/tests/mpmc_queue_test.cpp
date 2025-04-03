#include "../include/mpmc_queue.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>

// Basic functionality tests
TEST(MPMCQueueTest, BasicOperations) {
    MPMCQueue<int, 16> queue;
    
    // Test empty checks
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
    
    // Test enqueue
    EXPECT_TRUE(queue.enqueue(42));
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);
    
    // Test dequeue
    int value;
    EXPECT_TRUE(queue.dequeue(value));
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(queue.empty());
    
    // Test optional dequeue
    queue.enqueue(100);
    auto result = queue.dequeue();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 100);
    EXPECT_TRUE(queue.empty());
    
    // Test dequeue from empty queue
    EXPECT_FALSE(queue.dequeue(value));
    result = queue.dequeue();
    EXPECT_FALSE(result.has_value());
}

// Test filling the queue to capacity
TEST(MPMCQueueTest, FillingToCapacity) {
    constexpr size_t CAPACITY = 8;
    MPMCQueue<int, CAPACITY> queue;
    
    // Fill the queue
    for (size_t i = 0; i < CAPACITY; ++i) {
        EXPECT_TRUE(queue.enqueue(static_cast<int>(i)));
    }
    
    EXPECT_EQ(queue.size(), CAPACITY);
    
    // Try adding one more (should fail)
    EXPECT_FALSE(queue.enqueue(100));
    
    // Remove one item
    int value;
    EXPECT_TRUE(queue.dequeue(value));
    EXPECT_EQ(value, 0);
    
    // Now we should be able to add one
    EXPECT_TRUE(queue.enqueue(100));
    
    // Empty the queue and check values
    for (size_t i = 1; i < CAPACITY; ++i) {
        EXPECT_TRUE(queue.dequeue(value));
        EXPECT_EQ(value, static_cast<int>(i));
    }
    
    // Last item should be the one we added after removing one
    EXPECT_TRUE(queue.dequeue(value));
    EXPECT_EQ(value, 100);
    EXPECT_TRUE(queue.empty());
}

// Test wraparound behavior
TEST(MPMCQueueTest, Wraparound) {
    constexpr size_t CAPACITY = 4;
    MPMCQueue<int, CAPACITY> queue;
    int value;
    
    // Fill and drain the queue multiple times to test wraparound
    for (int iteration = 0; iteration < 10; ++iteration) {
        // Fill
        for (size_t i = 0; i < CAPACITY; ++i) {
            EXPECT_TRUE(queue.enqueue(static_cast<int>(i + iteration * 100)));
        }
        
        // Drain
        for (size_t i = 0; i < CAPACITY; ++i) {
            EXPECT_TRUE(queue.dequeue(value));
            EXPECT_EQ(value, static_cast<int>(i + iteration * 100));
        }
    }
    
    EXPECT_TRUE(queue.empty());
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

TEST(MPMCQueueTest, ComplexDataType) {
    MPMCQueue<TestObject, 4> queue;
    
    // Add objects
    TestObject obj1(1, "one");
    TestObject obj2(2, "two");
    
    EXPECT_TRUE(queue.enqueue(obj1));
    EXPECT_TRUE(queue.enqueue(obj2));
    EXPECT_EQ(queue.size(), 2);
    
    // Retrieve objects
    TestObject result;
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, obj1);
    
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, obj2);
    
    EXPECT_TRUE(queue.empty());
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

TEST(MPMCQueueTest, MoveOnlyType) {
    MPMCQueue<MoveOnlyType, 4> queue;
    
    EXPECT_TRUE(queue.enqueue(MoveOnlyType(42)));
    EXPECT_TRUE(queue.enqueue(MoveOnlyType(43)));
    
    auto result = queue.dequeue();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->getValue(), 42);
    
    MoveOnlyType value;
    EXPECT_TRUE(queue.dequeue(value));
    EXPECT_EQ(value.getValue(), 43);
}

TEST(MPMCQueueTest, MultiThreaded) {
    // Test parameters
    constexpr size_t NUM_PRODUCERS = 4;
    constexpr size_t NUM_CONSUMERS = 4;
    constexpr size_t NUM_ITEMS_PER_PRODUCER = 1000;
    constexpr size_t QUEUE_SIZE = 1024;

    // Create the MPMC queue
    MPMCQueue<int, QUEUE_SIZE> queue;

    // Atomic synchronization variables
    std::atomic<size_t> total_produced(0);
    std::atomic<size_t> total_consumed(0);
    std::atomic<bool> done(false);

    // Produce items
    auto producer_func = [&](size_t producer_id) {
        const size_t start_item = producer_id * NUM_ITEMS_PER_PRODUCER;
        const size_t end_item = start_item + NUM_ITEMS_PER_PRODUCER;
        
        for (size_t i = start_item; i < end_item; ++i) {
            int value = static_cast<int>(i);
            while (!queue.enqueue(value)) {
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
        
        while (total_consumed.load(std::memory_order_relaxed) < NUM_ITEMS_PER_PRODUCER * NUM_PRODUCERS) {
            int value;
            if (queue.dequeue(value)) {
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
    };

    // Create producer threads
    std::vector<std::thread> producers;
    for (size_t i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(producer_func, i);
    }

    // Create consumer threads
    std::vector<std::thread> consumers;
    for (size_t i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consumer_func);
    }

    // Wait for all producers to complete
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
    EXPECT_EQ(total_produced.load(), NUM_ITEMS_PER_PRODUCER * NUM_PRODUCERS) 
        << "Not all items were produced. Produced: " << total_produced.load();
    
    EXPECT_EQ(total_consumed.load(), total_produced.load()) 
        << "Consumed items (" << total_consumed.load() 
        << ") do not match produced items (" << total_produced.load() << ")";
    
    EXPECT_TRUE(queue.empty()) 
        << "Queue should be empty after processing all items";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
