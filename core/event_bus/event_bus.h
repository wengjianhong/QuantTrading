#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <typeindex>
#include "event.h"

namespace quant {
namespace core {
namespace event_bus {

// 事件总线接口
class EventBus {
public:
    // 单例模式
    static EventBus& instance();
    
    // 禁止拷贝和移动
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;
    
    // 订阅事件
    template <typename EventType>
    void subscribe(std::function<void(const EventType&)> handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto type = std::type_index(typeid(EventType));
        handlers_[type].push_back([handler](const Event& event) {
            handler(static_cast<const EventType&>(event));
        });
    }
    
    // 发布事件
    template <typename EventType>
    void publish(const EventType& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto type = std::type_index(typeid(EventType));
        auto it = handlers_.find(type);
        if (it != handlers_.end()) {
            for (const auto& handler : it->second) {
                handler(event);
            }
        }
    }
    
private:
    EventBus() = default;
    
    std::mutex mutex_;
    using Handler = std::function<void(const Event&)>;
    std::unordered_map<std::type_index, std::vector<Handler>> handlers_;
};

} // namespace event_bus
} // namespace core
} // namespace quant
