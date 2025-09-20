// 无锁队列实现，基于Michael-Scott算法
// 支持多生产者-多消费者场景，线程安全
#ifndef SHARED_BASE_LOCK_FREE_LOCK_FREE_QUEUE_H_
#define SHARED_BASE_LOCK_FREE_LOCK_FREE_QUEUE_H_

#include <atomic>
#include <memory>
#include <cassert>

namespace quant {
namespace shared {
namespace base {
namespace lock_free {

// 无锁队列节点
template <typename T>
struct Node {
    std::unique_ptr<T> data;  // 存储的数据
    std::atomic<Node*> next;  // 下一个节点的原子指针

    Node() : next(nullptr) {}
    explicit Node(T value) : data(std::make_unique<T>(std::move(value))), next(nullptr) {}
};

// 无锁队列实现
template <typename T>
class LockFreeQueue {
public:
    // 构造函数：初始化哨兵节点
    LockFreeQueue() : head_(new Node<T>()), tail_(head_.load()) {}

    // 禁止拷贝构造和赋值
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;

    // 移动构造和赋值
    LockFreeQueue(LockFreeQueue&& other) noexcept {
        head_.store(other.head_.exchange(nullptr));
        tail_.store(other.tail_.exchange(nullptr));
    }

    LockFreeQueue& operator=(LockFreeQueue&& other) noexcept {
        if (this != &other) {
            // 释放当前队列资源
            clear();
            
            // 接管其他队列的资源
            head_.store(other.head_.exchange(nullptr));
            tail_.store(other.tail_.exchange(nullptr));
        }
        return *this;
    }

    // 析构函数：清理所有节点
    ~LockFreeQueue() {
        clear();
        
        // 释放哨兵节点
        Node<T>* head = head_.load();
        if (head) {
            delete head;
        }
    }

    // 入队操作：将元素添加到队列尾部
    // 支持移动语义，避免拷贝开销
    void enqueue(T value) {
        Node<T>* new_node = new Node<T>(std::move(value));
        Node<T>* old_tail = tail_.load(std::memory_order_relaxed);

        while (true) {
            // 确保尾节点的next为nullptr
            Node<T>* next = old_tail->next.load(std::memory_order_acquire);
            if (next != nullptr) {
                // 尾节点已被其他线程更新，尝试推进尾指针
                tail_.compare_exchange_weak(old_tail, next,
                                           std::memory_order_release,
                                           std::memory_order_relaxed);
                continue;
            }

            // 尝试将新节点设置为尾节点的next
            if (old_tail->next.compare_exchange_weak(next, new_node,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed)) {
                // 成功添加新节点，尝试更新尾指针
                tail_.compare_exchange_weak(old_tail, new_node,
                                           std::memory_order_release,
                                           std::memory_order_relaxed);
                return;
            }
        }
    }

    // 出队操作：从队列头部取出元素
    // 成功返回true并将结果存入value，失败返回false（队列为空）
    bool dequeue(T& value) {
        Node<T>* old_head = head_.load(std::memory_order_relaxed);
        
        while (true) {
            if (old_head == nullptr) {
                return false;  // 队列为空
            }

            Node<T>* next = old_head->next.load(std::memory_order_acquire);
            if (next == nullptr) {
                return false;  // 队列为空
            }

            // 尝试获取头节点的所有权
            if (!head_.compare_exchange_weak(old_head, next,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
                continue;
            }

            // 成功获取元素，转移数据所有权
            value = std::move(*next->data);
            
            // 释放旧的头节点（原哨兵节点）
            delete old_head;
            return true;
        }
    }

    // 检查队列是否为空
    bool empty() const {
        Node<T>* head = head_.load(std::memory_order_acquire);
        Node<T>* next = head->next.load(std::memory_order_acquire);
        return next == nullptr;
    }

    // 清空队列
    void clear() {
        T dummy;
        while (dequeue(dummy)) {}
    }

private:
    // 头指针和尾指针，始终指向哨兵节点
    // 实际数据存储在头节点的next开始的节点中
    std::atomic<Node<T>*> head_;
    std::atomic<Node<T>*> tail_;
};

}  // namespace lock_free
}  // namespace base
}  // namespace shared
}  // namespace quant

#endif  // SHARED_BASE_LOCK_FREE_LOCK_FREE_QUEUE_H_
