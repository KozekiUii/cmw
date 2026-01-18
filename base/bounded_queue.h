#ifndef CMW_BASE_BOUNDED_QUEUE_H_
#define CMW_BASE_BOUNDED_QUEUE_H_


#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <utility>



#include <cmw/base/macros.h>
#include <cmw/base/wait_strategy.h>

namespace hnu    {
namespace cmw   {
namespace base {


template <typename T>
class BoundedQueue {
 public:
  using value_type = T;
  using size_type = uint64_t;

 public:
  BoundedQueue() {}
  BoundedQueue& operator=(const BoundedQueue& other) = delete;
  BoundedQueue(const BoundedQueue& other) = delete;
  ~BoundedQueue();
  bool Init(uint64_t size);
  bool Init(uint64_t size, WaitStrategy* strategy);
  bool Enqueue(const T& element);
  bool Enqueue(T&& element);
  bool WaitEnqueue(const T& element);
  bool WaitEnqueue(T&& element);
  bool Dequeue(T* element);
  bool WaitDequeue(T* element);
  uint64_t Size();
  bool Empty();
  void SetWaitStrategy(WaitStrategy* WaitStrategy);
  void BreakAllWait();
  uint64_t Head() { return head_.load(); }
  uint64_t Tail() { return tail_.load(); }
  uint64_t Commit() { return commit_.load(); }

 private:
  uint64_t GetIndex(uint64_t num);

  alignas(CACHELINE_SIZE) std::atomic<uint64_t> head_ = {0};    // 读指针的位置
  alignas(CACHELINE_SIZE) std::atomic<uint64_t> tail_ = {1};    // 下一个要写的位置
  alignas(CACHELINE_SIZE) std::atomic<uint64_t> commit_ = {1};  // 已提交的数据( < commit 可读)
  // alignas(CACHELINE_SIZE) std::atomic<uint64_t> size_ = {0};

  uint64_t pool_size_ = 0;
  T* pool_ = nullptr;
  std::unique_ptr<WaitStrategy> wait_strategy_ = nullptr;
  volatile bool break_all_wait_ = false;  // break all wait
};

template <typename T>
BoundedQueue<T>::~BoundedQueue() {
  if (wait_strategy_) {
    BreakAllWait();
  }
  if (pool_) {
    for (uint64_t i = 0; i < pool_size_; ++i) {
      pool_[i].~T();
    }
    std::free(pool_);
  }
}

/* 默认线程阻塞策略为睡眠策略 */
template <typename T>
inline bool BoundedQueue<T>::Init(uint64_t size) {
  return Init(size, new SleepWaitStrategy());
}

/* 指定队列大小和线程阻塞策略 */
template <typename T>
bool BoundedQueue<T>::Init(uint64_t size, WaitStrategy* strategy) {
  // Head and tail each occupy a space
  pool_size_ = size + 2;
  // reinterpret_cat<T*> = (T*)，Force pointer type conversion
  pool_ = reinterpret_cast<T*>(std::calloc(pool_size_, sizeof(T)));
  if (pool_ == nullptr) {
    return false;
  }
  for (uint64_t i = 0; i < pool_size_; ++i) {
    // placement new：don't allocate memory, just construct the object in the given memory location
    new (&(pool_[i])) T();
  }
  /* if wait_strategy_ used to have a value, it will be deleted and the new strategy will be constructed*/
  wait_strategy_.reset(strategy);
  return true;
}

// 核心入队函数，实现无锁多生产者并发入队，立即入队
template <typename T>
bool BoundedQueue<T>::Enqueue(const T& element) {
  uint64_t new_tail = 0;
  uint64_t old_commit = 0;
  uint64_t old_tail = tail_.load(std::memory_order_acquire);
  do {
    new_tail = old_tail + 1;
    if (GetIndex(new_tail) == GetIndex(head_.load(std::memory_order_acquire))) {
      return false;
    }
  } while (!tail_.compare_exchange_weak(old_tail, new_tail,
                                        std::memory_order_acq_rel,
                                        std::memory_order_relaxed));
  pool_[GetIndex(old_tail)] = element;
  do {
    old_commit = old_tail;
  } while (cyber_unlikely(!commit_.compare_exchange_weak(
      old_commit, new_tail, std::memory_order_acq_rel,
      std::memory_order_relaxed)));
  wait_strategy_->NotifyOne();
  return true;
}

// 使用移动赋值运算的入队函数，立即入队
template <typename T>
bool BoundedQueue<T>::Enqueue(T&& element) {
  uint64_t new_tail = 0;
  uint64_t old_commit = 0;
  uint64_t old_tail = tail_.load(std::memory_order_acquire);
  do {
    new_tail = old_tail + 1;
    if (GetIndex(new_tail) == GetIndex(head_.load(std::memory_order_acquire))) {
      return false;
    }
  } while (!tail_.compare_exchange_weak(old_tail, new_tail,
                                        std::memory_order_acq_rel,
                                        std::memory_order_relaxed));
  // 使用std::move将element这个左值引用（因为有名字）还原成右值引用，随后调用了T的移动赋值运算
  pool_[GetIndex(old_tail)] = std::move(element);
  do {
    old_commit = old_tail;
  } while (cyber_unlikely(!commit_.compare_exchange_weak(
      old_commit, new_tail, std::memory_order_acq_rel,
      std::memory_order_relaxed)));
  wait_strategy_->NotifyOne();
  return true;
}

// 出队函数，无锁多消费者出队
template <typename T>
bool BoundedQueue<T>::Dequeue(T* element) {
  uint64_t new_head = 0;
  uint64_t old_head = head_.load(std::memory_order_acquire);
  do {
    new_head = old_head + 1;
    // 这里比较的是 commit_ 而不是 tail_。因为生产者可能已经占了位置（推进了 tail_），但数据还没写完（commit_ 未推进）。
    if (new_head == commit_.load(std::memory_order_acquire)) {
      return false;
    }
    // 在CAS之前读取数据，因为环形队列可能会在CAS成功后，tail_将new_head覆盖
    *element = pool_[GetIndex(new_head)];
  } while (!head_.compare_exchange_weak(old_head, new_head,
                                        std::memory_order_acq_rel,
                                        std::memory_order_relaxed));
  return true;
}

/*基于等待策略的入队操作*/
template <typename T>
bool BoundedQueue<T>::WaitEnqueue(const T& element) {
  while (!break_all_wait_) {
    if (Enqueue(element)) {
      return true;
    }
    if (wait_strategy_->EmptyWait()) {
      continue;
    }
    // wait timeout
    break;
  }

  return false;
}

/*基于等待策略的入队操作(移动赋值运算)*/
template <typename T>
bool BoundedQueue<T>::WaitEnqueue(T&& element) {
  while (!break_all_wait_) {
    if (Enqueue(std::move(element))) {
      return true;
    }
    if (wait_strategy_->EmptyWait()) {
      continue;
    }
    // wait timeout
    break;
  }

  return false;
}

/*基于等待策略的出队操作*/
template <typename T>
bool BoundedQueue<T>::WaitDequeue(T* element) {
  while (!break_all_wait_) {
    /*如果对了里有数据，则直接return true，否则返回false*/
    if (Dequeue(element)) {
      return true;
    }
    /*执行等待策略*/
    if (wait_strategy_->EmptyWait()) {
      continue;
    }
    // wait timeout
    break;
  }

  return false;
}

// 获取队列大小
template <typename T>
inline uint64_t BoundedQueue<T>::Size() {
  return tail_ - head_ - 1;
}

// 判断队列是否为空
template <typename T>
inline bool BoundedQueue<T>::Empty() {
  return Size() == 0;
}

/* 由于是无符号整数，所以返回的是索引，类似于取余*/
template <typename T>
inline uint64_t BoundedQueue<T>::GetIndex(uint64_t num) {
  return num - (num / pool_size_) * pool_size_;  // faster than %，equal to -> num % pool_size_
}

// 设置等待策略
template <typename T>
inline void BoundedQueue<T>::SetWaitStrategy(WaitStrategy* strategy) {
  wait_strategy_.reset(strategy);
}

// 设置break_all_wait_为true，表示所有等待的线程都应该退出
// 并调用wait_strategy_->BreakAllWait()，通知所有等待的线程退出
// 用于在队列被销毁时，通知所有等待的线程退出
template <typename T>
inline void BoundedQueue<T>::BreakAllWait() {
  break_all_wait_ = true;
  wait_strategy_->BreakAllWait();
}


}
}
}









#endif
