#include <gtest/gtest.h>
#include <vector>
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include "base/common/thread_pool/thread_pool.h"

using namespace quant::base::common::thread_pool;
using namespace std::chrono_literals;

// 基本功能测试
TEST(ThreadPoolTest, BasicFunctionality) {
    auto pool = ThreadPool::create(2);
    EXPECT_EQ(pool->thread_count(), 2);
    EXPECT_TRUE(pool->is_running());
    
    // 提交简单任务
    std::atomic<int> counter(0);
    pool->submit([&counter]() { counter++; });
    pool->submit([&counter]() { counter++; });
    
    // 等待任务完成
    pool->wait_all();
    EXPECT_EQ(counter, 2);
    std::cout << "BasicFunctionality completed." << std::endl;
}

// 带返回值的任务测试
TEST(ThreadPoolTest, TasksWithReturnValues) {
    auto pool = ThreadPool::create(4);
    
    // 提交多个带返回值的任务
    auto future1 = pool->submit([]() { return 10; });
    auto future2 = pool->submit([]() { return 20; });
    auto future3 = pool->submit([]() { return 30; });
    
    // 获取结果
    EXPECT_EQ(future1.get(), 10);
    EXPECT_EQ(future2.get(), 20);
    EXPECT_EQ(future3.get(), 30);
}


// 多线程任务提交测试
TEST(ThreadPoolTest, MultipleThreadsSubmitting) {
    auto pool = ThreadPool::create(8);
    const int kNumTasks = 10000;
    std::atomic<int> counter(0);
    std::vector<std::thread> submitters;
    std::vector<std::future<void>> futures;
    std::mutex futures_mutex;  // 新增：保护futures的并发修改

    for (int i = 0; i < 4; ++i) {
        // 捕获futures_mutex（值捕获，mutex不可移动，需用引用包装）
        submitters.emplace_back([&, pool]() {
            for (int j = 0; j < kNumTasks; ++j) {
                auto fut = pool->submit([&counter]() { counter++; });
                // 加锁保护push_back，确保线程安全
                std::lock_guard<std::mutex> lock(futures_mutex);
                futures.push_back(std::move(fut));
            }
        });
    }

    // 等待所有提交者完成
    for (auto& t : submitters) {
        t.join();  // 无需joinable()检查，线程必为可join状态
    }


    // 等待所有任务完成（future.wait()已足够，无需再调用pool->wait_all()）
    for (auto& fut : futures) {
        fut.wait();
    }

    // 验证计数（此时counter必然等于4*kNumTasks）
    EXPECT_EQ(counter, 4 * kNumTasks);
}

// 异常处理测试
TEST(ThreadPoolTest, ExceptionHandling) {
    auto pool = ThreadPool::create(2);
    
    // 提交会抛出异常的任务
    auto future = pool->submit([]() {
        throw std::runtime_error("Test exception");
    });
    
    // 验证异常被正确捕获并传递
    EXPECT_THROW(future.get(), std::runtime_error);
    
    // 验证线程池仍能正常工作
    std::atomic<bool> completed(false);
    pool->submit([&completed]() { completed = true; });
    pool->wait_all();
    EXPECT_TRUE(completed);
}

// 线程池停止测试
TEST(ThreadPoolTest, StopBehavior) {
    // 测试1：等待所有任务完成
    {
        auto pool = ThreadPool::create(2);
        std::atomic<int> counter(0);
        
        for (int i = 0; i < 100; ++i) {
            pool->submit([&counter]() { 
                std::this_thread::sleep_for(1ms);
                counter++; 
            });
        }
        
        pool->stop(true);
        EXPECT_EQ(counter, 100);
        EXPECT_FALSE(pool->is_running());
    }
    
    // 测试2：不等待任务完成
    {
        auto pool = ThreadPool::create(2);
        std::atomic<int> counter(0);
        
        for (int i = 0; i < 100; ++i) {
            pool->submit([&counter]() { 
                std::this_thread::sleep_for(100ms);
                counter++; 
            });
        }
        
        // 立即停止，不等待所有任务完成
        pool->stop(false);
        EXPECT_LT(counter, 100);  // 应该有部分任务未执行
        EXPECT_FALSE(pool->is_running());
    }
}

// 性能测试
TEST(ThreadPoolTest, Performance) {
    const int kNumTasks = 1000000;
    auto pool = ThreadPool::create(std::thread::hardware_concurrency());
    
    // 测试任务提交和执行性能
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::future<void>> futures;
    futures.reserve(kNumTasks);
    
    for (int i = 0; i < kNumTasks; ++i) {
        futures.push_back(pool->submit([]() { 
            // 简单的计算任务
            volatile int x = 0;
            for (int j = 0; j < 10; ++j) x++;
        }));
    }
    
    // 等待所有任务完成
    for (auto& f : futures) {
        f.get();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "ThreadPool Performance: " << std::endl;
    std::cout << "  Completed " << kNumTasks << " tasks in " << duration << "ms" << std::endl;
    std::cout << "  Tasks per second: " << (kNumTasks * 1000 / duration) << std::endl;
}

// 任务计数测试
TEST(ThreadPoolTest, TaskCounting) {
    auto pool = ThreadPool::create(2);
    EXPECT_EQ(pool->pending_tasks(), 0);
    
    // 提交一批任务但不等待完成
    const int kNumTasks = 100;
    for (int i = 0; i < kNumTasks; ++i) {
        pool->submit([]() { std::this_thread::sleep_for(1ms); });
    }
    
    // 等待任务进入队列
    std::this_thread::sleep_for(10ms);
    
    // 任务计数应该接近kNumTasks（可能已有部分完成）
    EXPECT_GT(pool->pending_tasks(), 0);
    EXPECT_LE(pool->pending_tasks(), kNumTasks);
    
    // 等待所有任务完成
    pool->wait_all();
    EXPECT_EQ(pool->pending_tasks(), 0);
}
