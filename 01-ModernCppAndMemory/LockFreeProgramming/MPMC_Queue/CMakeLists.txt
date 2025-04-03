cmake_minimum_required(VERSION 3.16)
project(MPMC_Queue VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable all warnings
if(MSVC)
    # Disable specific warnings
    add_compile_options(/W4 /wd4324)  # Disable padding warning 4324
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Enable optimization for Release builds
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Add the executable
add_executable(mpmc_queue_demo src/main.cpp)
target_include_directories(mpmc_queue_demo PRIVATE include)

# Find Google Test
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
endif()

# Add the test executable
add_executable(mpmc_queue_test tests/mpmc_queue_test.cpp)
target_include_directories(mpmc_queue_test PRIVATE include)
target_link_libraries(mpmc_queue_test PRIVATE GTest::gtest GTest::gtest_main)

# Find Google Benchmark
find_package(benchmark QUIET)
if(NOT benchmark_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.7.1
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing" FORCE)
    FetchContent_MakeAvailable(googlebenchmark)
endif()

# Add the benchmark executable
add_executable(mpmc_queue_bench benchmarks/mpmc_queue_bench.cpp)
target_include_directories(mpmc_queue_bench PRIVATE include)
target_link_libraries(mpmc_queue_bench PRIVATE benchmark::benchmark)

# Add pthread on Unix-like systems
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(mpmc_queue_demo PRIVATE Threads::Threads)
    target_link_libraries(mpmc_queue_test PRIVATE Threads::Threads)
    target_link_libraries(mpmc_queue_bench PRIVATE Threads::Threads)
endif()

# Enable testing
enable_testing()
add_test(NAME MPMCQueueTest COMMAND mpmc_queue_test)
add_test(NAME MPMCQueueBenchmark COMMAND mpmc_queue_bench)

# Install targets
install(TARGETS mpmc_queue_demo mpmc_queue_test mpmc_queue_bench
        RUNTIME DESTINATION bin
)

# Install header files
install(FILES include/mpmc_queue.h
        DESTINATION include
)
