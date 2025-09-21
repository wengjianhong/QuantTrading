#ifndef BASE_COMMON_THREAD_POOL_THREAD_POOL_H_
#define BASE_COMMON_THREAD_POOL_THREAD_POOL_H_

#include <mutex>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <future>
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <type_traits>
#include <condition_variable>
#include "../safe_queue/safe_queue.h"  // 引入无锁队列

namespace quant {
namespace base {
namespace common {
namespace thread_pool {

// 线程池：基于无锁队列，支持任务提交与等待
class ThreadPool : public std::enable_shared_from_this<ThreadPool> {
public:
    // 禁止拷贝/赋值
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 工厂方法：强制通过 shared_ptr 创建，避免栈上对象析构风险
    static std::shared_ptr<ThreadPool> create(size_t thread_count = std::thread::hardware_concurrency()) {
        if (thread_count == 0) {
            throw std::invalid_argument("Thread count must be greater than 0");
        }
        return std::shared_ptr<ThreadPool>(new ThreadPool(thread_count));
    }

    // 析构：自动停止线程池，确保任务完成
    ~ThreadPool() {
        stop(true);
    }

    // 提交任务：支持任意参数的函数，返回 future 用于获取结果
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        if (!is_running_.load(std::memory_order_acquire)) {
            throw std::runtime_error("ThreadPool is stopped");
        }

        using ReturnType = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<ReturnType> result = task->get_future();

        task_count_.fetch_add(1, std::memory_order_acq_rel);
        auto self = shared_from_this();

        // 关键：捕获 task 的副本，避免嵌套引用导致的生命周期混乱
        task_queue_.push([task, self]() {
            try {
                if (task) {  // 仅需判断 shared_ptr 是否有效
                    (*task)();
                }
            } catch (const std::exception& e) {
                std::cerr << "Task error: " << e.what() << std::endl;
            }

            // 递减任务计数
            size_t remaining = self->task_count_.fetch_sub(1, std::memory_order_acq_rel);
            if (remaining == 1) {
                std::lock_guard<std::mutex> lock(self->wait_mutex_);
                self->wait_cv_.notify_all();
            }
        });

        return result;
    }

    // 等待所有任务完成（阻塞直到 task_count_ 为 0）
    void wait_all() {
        std::unique_lock<std::mutex> lock(wait_mutex_);
        wait_cv_.wait(lock, [this]() {
            return task_count_.load(std::memory_order_acquire) == 0;
        });
    }

    // 获取当前等待的任务数（近似值，包含已出队但未执行的任务）
    size_t pending_tasks() const {
        return task_count_.load(std::memory_order_acquire);
    }

    // 获取线程池的线程数量
    size_t thread_count() const {
        return threads_.size();
    }

    // 停止线程池：wait_for_completion=true 表示等待所有任务完成
    void stop(bool wait_for_completion = true) {
        // 原子操作：标记线程池为停止状态（仅第一次调用有效）
        if (!is_running_.exchange(false, std::memory_order_acq_rel)) {
            return;
        }

        // 如果不等待完成，清空任务队列
        if (!wait_for_completion) {
            clear_queue();
        }

        // 等待所有任务完成（可选）
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

    // 检查线程池是否正在运行
    bool is_running() const {
        return is_running_.load(std::memory_order_acquire);
    }

private:
    // 私有构造：仅允许通过 create() 工厂方法创建
    explicit ThreadPool(size_t thread_count) 
        : is_running_(true), task_count_(0), wait_for_completion_(true) {
        // 创建工作线程
        threads_.reserve(thread_count);
        for (size_t i = 0; i < thread_count; ++i) {
            threads_.emplace_back(&ThreadPool::worker_thread, this);
        }
    }

    // 清空任务队列
    void clear_queue() {
        std::function<void()> dummy;
        while (task_queue_.pop(dummy)) {
            // 递减任务计数，因为我们正在丢弃这些任务
            task_count_.fetch_sub(1, std::memory_order_acq_rel);
        }
        // 通知可能在等待的线程
        std::lock_guard<std::mutex> lock(wait_mutex_);
        wait_cv_.notify_all();
    }

    // 工作线程逻辑：循环从队列取任务执行
    void worker_thread() {
        while (is_running_.load(std::memory_order_acquire)) {
            std::function<void()> task;
            // 从无锁队列取任务（队列为空时 yield，避免忙等）
            if (task_queue_.pop(task)) {
                try {
                    // 执行任务前检查有效性
                    if (task) {
                        task();
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[WorkerThread] Task execution error: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "[WorkerThread] Unknown task error" << std::endl;
                }
            } else {
                std::this_thread::yield();
            }
        }

        // 只有当需要等待完成时，才处理队列中剩余的任务
        if (wait_for_completion_) {
            std::function<void()> remaining_task;
            while (task_queue_.pop(remaining_task)) {
                try {
                    if (remaining_task) {
                        remaining_task();
                    }
                } catch (...) {
                    // 停止后忽略异常
                }
            }
        }
    }

private:
    bool wait_for_completion_;                                  // 是否等待任务完成的标志
    std::atomic<bool> is_running_;                              // 线程池运行状态
    std::atomic<size_t> task_count_;                            // 未完成任务计数
    std::mutex wait_mutex_;                                     // wait_all() 同步锁
    std::condition_variable wait_cv_;                           // wait_all() 条件变量
    std::vector<std::thread> threads_;                          // 工作线程列表
    safe_queue::SafeQueue<std::function<void()>> task_queue_;   // 任务队列（无锁）
};

}  // namespace thread_pool
}  // namespace common
}  // namespace base
}  // namespace quant

#endif  // BASE_COMMON_THREAD_POOL_THREAD_POOL_H_
