#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <future>
#include <type_traits>
#include "../lock_free/lock_free_queue.h"

namespace quant::shared::base::thread_pool {

// 基于无锁队列的线程池实现
class ThreadPool {
public:
    // 构造函数：指定线程数量，默认使用硬件核心数
    explicit ThreadPool(size_t thread_count = std::thread::hardware_concurrency());
    
    // 析构函数：停止线程池
    ~ThreadPool();
    
    // 禁用拷贝构造和赋值
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    // 提交任务并返回future
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    
    // 等待所有任务完成
    void wait_all();
    
    // 获取当前等待的任务数量
    size_t pending_tasks() const;
    
    // 获取线程数量
    size_t thread_count() const { return threads_.size(); }
    
    // 停止线程池（可选参数：是否等待所有任务完成）
    void stop(bool wait_for_completion = true);
    
    // 检查线程池是否正在运行
    bool is_running() const { return is_running_.load(std::memory_order_acquire); }

private:
    // 工作线程函数
    void worker_thread();
    
    // 任务类型（包装为无参数函数）
    using Task = std::function<void()>;
    
    // 工作线程
    std::vector<std::thread> threads_;
    
    // 任务队列（无锁队列）
    lock_free::LockFreeQueue<Task> task_queue_;
    
    // 原子标志：线程池是否运行
    std::atomic<bool> is_running_;
    
    // 原子计数器：等待的任务数量
    std::atomic<size_t> task_count_;
    
    // 用于等待所有任务完成的信号量
    std::mutex wait_mutex_;
    std::condition_variable wait_cv_;
};

// 提交任务的模板实现
template<typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    // 检查线程池是否正在运行
    if (!is_running_.load(std::memory_order_acquire)) {
        throw std::runtime_error("ThreadPool is not running");
    }
    
    // 用packaged_task包装任务，以便获取future
    using ReturnType = typename std::result_of<F(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    // 获取future用于返回
    std::future<ReturnType> result = task->get_future();
    
    // 增加任务计数
    task_count_.fetch_add(1, std::memory_order_relaxed);
    
    // 将任务放入队列（包装为无参数函数，并在执行后减少任务计数）
    task_queue_.enqueue([task, this]() {
        try {
            (*task)();
        } catch (...) {
            // 捕获任务执行中的异常，避免线程终止
            // （异常会被存储在future中，由调用者处理）
        }
        
        // 减少任务计数，如果为0则通知等待者
        size_t remaining = task_count_.fetch_sub(1, std::memory_order_acq_rel);
        if (remaining == 1) {
            std::lock_guard<std::mutex> lock(wait_mutex_);
            wait_cv_.notify_all();
        }
    });
    
    return result;
}

}  // namespace quant::shared::base::thread_pool
