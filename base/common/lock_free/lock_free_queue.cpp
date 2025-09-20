// 无锁队列的实现文件
// 模板类的实现通常放在头文件中，但为了演示分离编译结构保留此文件
#include "lock_free_queue.h"

namespace quant {
namespace shared {
namespace base {
namespace lock_free {

// 模板类的显式实例化声明，用于分离编译
// 实际使用时可根据需要添加常用类型的实例化
template class LockFreeQueue<int>;
template class LockFreeQueue<long>;
template class LockFreeQueue<std::string>;

}  // namespace lock_free
}  // namespace base
}  // namespace shared
}  // namespace quant
