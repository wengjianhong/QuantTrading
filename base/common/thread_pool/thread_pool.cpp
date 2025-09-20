#include "thread_pool.h"
#include <stdexcept>
#include <iostream>
#include <this_thread>

namespace quant::shared::base::thread_pool {

ThreadPool::ThreadPool(size_t thread_count) 
    : is_running_(true), task_count_(0) {
    if (thread_count == 0) {
        throw std::invalid_argument("Thread count must be greater than 0");
    }
    
    // 创建工作线程
    threads_.reserve(thread_count);
    for (size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back(&ThreadPool::worker_thread, this);
    }
}

ThreadPool::~ThreadPool() {
    stop(true);
}

void ThreadPool::worker_thread() {
    // 线程循环：从队列获取任务并执行，直到线程池停止
    while (is_running_.load(std::memory_order_acquire)) {
        Task task;
        
        // 尝试从队列获取任务
        if (task_queue_.dequeue(task)) {
            try {
                // 执行任务
                task();
            } catch (const std::exception& e) {
                // 记录任务执行异常
                std::cerr << "Task execution error: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown error in task execution" << std::endl;
            }
        } else {
            // 队列为空时短暂休眠，避免忙等
            std::this_thread::yield();
        }
    }
    
    // 线程池停止后，处理剩余任务（如果还有）
    if (!is_running_.load(std::memory_order_acquire)) {
        Task task;
        while (task_queue_.dequeue(task)) {
            try {
                task();
            } catch (...) {
                // 忽略停止后的任务异常
            }
        }
    }
}

void ThreadPool::wait_all() {
    std::unique_lock<std::mutex> lock(wait_mutex_);
    wait_cv_.wait(lock, [this]() { 
        return task_count_.load(std::memory_order_acquire) == 0; 
    });
}

size_t ThreadPool::pending_tasks() const {
    // 注意：task_count_包含已出队但未完成的任务
    // 这里返回的值是一个近似值，因为无锁队列的size()难以精确计算
    return task_count_.load(std::memory_order_acquire);
}

void ThreadPool::stop(bool wait_for_completion) {
    if (!is_running_.exchange(false, std::memory_order_acq_rel)) {
        return;  // 已经停止
    }
    
    // 如果需要等待所有任务完成
    if (wait_for_completion) {
        wait_all();
    }
    
    // 等待所有工作线程退出
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    threads_.clear();
}

}  // namespace quant::shared::base::thread_pool
