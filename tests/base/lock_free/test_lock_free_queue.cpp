#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <numeric>
#include "shared/base/lock_free/lock_free_queue.h"

using namespace quant::shared::base::lock_free;

// 基本功能测试
TEST(LockFreeQueueTest, BasicOperations) {
    LockFreeQueue<int> queue;
    
    // 初始状态应为空
    EXPECT_TRUE(queue.empty());
    
    // 入队操作
    queue.enqueue(10);
    queue.enqueue(20);
    EXPECT_FALSE(queue.empty());
    
    // 出队操作
    int value;
    EXPECT_TRUE(queue.dequeue(value));
    EXPECT_EQ(value, 10);
    
    EXPECT_TRUE(queue.dequeue(value));
    EXPECT_EQ(value, 20);
    
    // 队列应为空
    EXPECT_FALSE(queue.dequeue(value));
    EXPECT_TRUE(queue.empty());
}

// 移动语义测试
TEST(LockFreeQueueTest, MoveSemantics) {
    LockFreeQueue<std::string> queue;
    std::string str = "test string";
    
    // 测试移动入队
    queue.enqueue(std::move(str));
    EXPECT_TRUE(str.empty());  // 原字符串应被移动
    
    // 测试移动出队
    std::string result;
    EXPECT_TRUE(queue.dequeue(result));
    EXPECT_EQ(result, "test string");
}

// 多生产者单消费者测试
TEST(LockFreeQueueTest, MultipleProducers) {
    LockFreeQueue<int> queue;
    const int kNumProducers = 4;
    const int kItemsPerProducer = 1000;
    std::vector<std::thread> producers;
    
    // 启动多个生产者线程
    for (int i = 0; i < kNumProducers; ++i) {
        producers.emplace_back([&queue, i, kItemsPerProducer]() {
            for (int j = 0; j < kItemsPerProducer; ++j) {
                queue.enqueue(i * kItemsPerProducer + j);
            }
        });
    }
    
    // 等待所有生产者完成
    for (auto& t : producers) {
        t.join();
    }
    
    // 单消费者消费所有元素
    std::vector<int> results;
    int value;
    while (queue.dequeue(value)) {
        results.push_back(value);
    }
    
    // 验证元素总数正确
    EXPECT_EQ(results.size(), kNumProducers * kItemsPerProducer);
    
    // 验证所有元素都被正确生产和消费
    std::sort(results.begin(), results.end());
    for (int i = 0; i < results.size(); ++i) {
        EXPECT_EQ(results[i], i);
    }
}

// 多生产者多消费者测试
TEST(LockFreeQueueTest, MultipleProducersConsumers) {
    LockFreeQueue<int> queue;
    const int kNumProducers = 4;
    const int kNumConsumers = 2;
    const int kItemsPerProducer = 1000;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<int> total_consumed(0);
    std::atomic<bool> done(false);
    
    // 启动生产者
    for (int i = 0; i < kNumProducers; ++i) {
        producers.emplace_back([&queue, kItemsPerProducer]() {
            for (int j = 0; j < kItemsPerProducer; ++j) {
                queue.enqueue(j);
            }
        });
    }
    
    // 启动消费者
    for (int i = 0; i < kNumConsumers; ++i) {
        consumers.emplace_back([&]() {
            int value;
            while (!done || !queue.empty()) {
                if (queue.dequeue(value)) {
                    total_consumed++;
                } else {
                    // 短暂休眠避免忙等
                    std::this_thread::yield();
                }
            }
        });
    }
    
    // 等待生产者完成
    for (auto& t : producers) {
        t.join();
    }
    
    // 通知消费者完成
    done = true;
    
    // 等待消费者完成
    for (auto& t : consumers) {
        t.join();
    }
    
    // 验证消费总数正确
    EXPECT_EQ(total_consumed, kNumProducers * kItemsPerProducer);
}

// 性能测试（可选）
TEST(LockFreeQueueTest, PerformanceTest) {
    LockFreeQueue<int> queue;
    const int kNumItems = 1000000;
    const int kNumThreads = std::thread::hardware_concurrency();
    
    // 测试入队性能
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([&queue, kNumItems, kNumThreads, i]() {
            int items_per_thread = kNumItems / kNumThreads;
            for (int j = 0; j < items_per_thread; ++j) {
                queue.enqueue(i * items_per_thread + j);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto enqueue_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // 测试出队性能
    start = std::chrono::high_resolution_clock::now();
    int value;
    int count = 0;
    while (queue.dequeue(value)) {
        count++;
    }
    end = std::chrono::high_resolution_clock::now();
    auto dequeue_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_EQ(count, kNumItems);
    
    // 输出性能指标（仅作参考）
    std::cout << "LockFreeQueue Performance:" << std::endl;
    std::cout << "  Enqueued " << kNumItems << " items in " << enqueue_time << "ms" << std::endl;
    std::cout << "  Dequeued " << kNumItems << " items in " << dequeue_time << "ms" << std::endl;
}
