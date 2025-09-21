#ifndef BASE_COMMON_SAFE_QUEUE_H_
#define BASE_COMMON_SAFE_QUEUE_H_

#include <deque>          // 底层存储（高效头尾操作）
#include <mutex>          // 互斥锁（保护临界区）
#include <utility>        // 用于 std::forward/move（移动语义）
#include <cstddef>        // 用于 size_t
#include <stdexcept>      // 用于异常定义
#include <condition_variable>  // 条件变量（空队列阻塞等待）

namespace quant {
namespace base {
namespace common {
namespace safe_queue {

// 线程安全的有锁队列（多生产者-多消费者支持）
template <typename T>
class SafeQueue {
public:
    // 1. 构造/析构：默认构造，禁止拷贝/移动（避免并发场景下的浅拷贝问题）
    SafeQueue() = default;
    ~SafeQueue() = default;

    // 禁止拷贝和移动（如需支持，需手动实现并加锁保护）
    SafeQueue(const SafeQueue&) = delete;
    SafeQueue& operator=(const SafeQueue&) = delete;
    SafeQueue(SafeQueue&&) = delete;
    SafeQueue& operator=(SafeQueue&&) = delete;


    // 2. 入队操作：支持左值（拷贝）和右值（移动），线程安全
    // 左值入队（拷贝语义）
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);  // RAII 锁：自动加锁/释放
        data_.push_back(value);                    // 底层容器入队
        cv_.notify_one();                          // 唤醒一个等待出队的线程（避免空队列阻塞）
    }

    // 右值入队（移动语义，减少拷贝开销）
    void push(T&& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.push_back(std::move(value));  // 移动而非拷贝，适合临时对象
        cv_.notify_one();
    }


    // 3. 出队操作：两种模式（阻塞等待/非阻塞尝试）
    // 非阻塞式出队：队列空时直接返回 false，不阻塞；成功返回 true 并赋值 value
    bool pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (data_.empty()) {
            return false;  // 队空，直接返回失败
        }

        // 弹出队头数据
        value = std::move(data_.front());
        data_.pop_front();
        return true;
    }

    // 阻塞式出队：队列空时阻塞，直到有数据入队，返回 true；成功后将值存入 value
    bool block_pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);  // 支持手动解锁（用于条件变量等待）
        
        // 处理「虚假唤醒」：必须用 while 而非 if（操作系统可能虚假唤醒线程）
        while (data_.empty()) {
            // 释放锁并阻塞，直到被 notify 唤醒（唤醒后重新加锁）
            cv_.wait(lock);
        }

        // 移动获取数据（减少拷贝），弹出队头
        value = std::move(data_.front());
        data_.pop_front();
        return true;
    }


    // 4. 队列状态查询：线程安全（需加锁保护）
    // 获取队列大小（瞬时值，高并发下可能有微小偏差）
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);  // mutex_ 需为 mutable（const 函数可修改）
        return data_.size();
    }

    // 判断队列是否为空（瞬时值）
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.empty();
    }


    // 5. 清空队列：线程安全
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.clear();  // 清空底层容器
        // 无需 notify：清空后队列仍为空，等待的线程会继续阻塞直到新数据入队
    }


    // 6. 批量入队（可选扩展：高效处理多个元素）
    template <typename InputIt>
    void push_bulk(InputIt first, InputIt last) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.insert(data_.end(), first, last);  // 批量插入
        if (data_.size() >= 1) {
            cv_.notify_all();  // 批量入队，唤醒所有等待线程
        }
    }

private:
    mutable std::mutex mutex_;                // 保护所有临界区（mutable 允许 const 函数加锁）
    std::condition_variable cv_;              // 用于「空队列阻塞等待」
    std::deque<T> data_;                      // 底层存储容器（头尾操作 O(1)）
};

}  // namespace safe_queue
}  // namespace common
}  // namespace base
}  // namespace quant

#endif  // BASE_COMMON_SAFE_QUEUE_H_
