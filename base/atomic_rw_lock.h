#ifndef HNU_BASE_ATOMIC_RW_LOCK_H_
#define HNU_BASE_ATOMIC_RW_LOCK_H_

#include <cmw/base/rw_lock_guard.h>
#include <mutex>

namespace hnu {
namespace cmw {
namespace base {

class AtomicRWLock {
  friend class ReadLockGuard<AtomicRWLock>;
  friend class WriteLockGuard<AtomicRWLock>;

public:
  static const int32_t RW_LOCK_FREE = 0;
  static const int32_t WRITE_EXCLUSIVE = -1;
  static const uint32_t MAX_RETRY_TIMES = 5;
  AtomicRWLock() {}
  explicit AtomicRWLock(bool write_first) : write_first_(write_first) {}

private:
  // all these function only can used by ReadLockGuard/WriteLockGuard;
  void ReadLock();
  void WriteLock();

  void ReadUnlock();
  void WriteUnlock();

  AtomicRWLock(const AtomicRWLock &) = delete;
  AtomicRWLock &operator=(const AtomicRWLock &) = delete;
  std::atomic<uint32_t> write_lock_wait_num_ = {0};
  std::atomic<int32_t> lock_num_ = {0};
  bool write_first_ = true;
};

inline void AtomicRWLock::ReadLock() {
  uint32_t retry_times = 0;
  // lock_num 存储锁的当前状态;;
  // lock_num > 0 ：有成员在进行读操作;;
  // lock_num < 0 ：有成员在进行写操作;;
  // lock_num = 0 ：写操作结束了,锁处于空闲状态;;
  // load is a atomic operation
  int32_t lock_num = lock_num_.load();
  
  if (write_first_) {
    do {
      // 若 lock_num < 0，说明有成员在进行写操作，
      // 若 write_lock_wait_num_ > 0，说明有成员在等待写操作
      // 若满足条件，则继续自旋等待，每5次让出1次CPU时间片，让给其他线程
      // 如果有线程在等待写锁，或者有线程在进行写操作，那么读线程就不能获取写锁
      while (lock_num < RW_LOCK_FREE || write_lock_wait_num_.load() > 0) {
        if (++retry_times == MAX_RETRY_TIMES) {
          // saving cpu
          std::this_thread::yield();
          retry_times = 0;
        }
        lock_num = lock_num_.load();
      }
    } while (!lock_num_.compare_exchange_weak(lock_num, lock_num + 1,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed));
  } else {
    do {
      // 若 lock_num < 0，说明有成员在进行写操作
      // 由于是读优先，所以读线程可以抢占等待的写操作，就不用判断是否有写线程在等待了
      while (lock_num < RW_LOCK_FREE) {
        if (++retry_times == MAX_RETRY_TIMES) {
          // saving cpu
          std::this_thread::yield();
          retry_times = 0;
        }
        lock_num = lock_num_.load();
      }
      // 如果在写线程结束后，有其他线程改变了lock_num_，那么compare_exchange_weak会失败，
      // 此时需要重新获取lock_num_，并重新尝试获取写锁
      // 比如：1. 另一个线程获取了写锁，lock_num_变为-1
      //      2. 另一个线程获取了读锁，lock_num_变为1
    } while (!lock_num_.compare_exchange_weak(lock_num, lock_num + 1,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed));
  }
}

inline void AtomicRWLock::WriteLock() {
  int32_t rw_lock_free = RW_LOCK_FREE;
  uint32_t retry_times = 0;
  // atomic operation
  // 等待获取写锁的线程数加 1，此时还未获取到写锁
  write_lock_wait_num_.fetch_add(1);
  // 尝试获取写锁，如果获取失败，则继续自旋等待，每5次让出1次CPU时间片，让给其他线程
  while (!lock_num_.compare_exchange_weak(rw_lock_free, WRITE_EXCLUSIVE,
                                          std::memory_order_acq_rel,
                                          std::memory_order_relaxed)) {
    // rw_lock_free will change after CAS fail, so init again
    rw_lock_free = RW_LOCK_FREE;
    if (++retry_times == MAX_RETRY_TIMES) {
      // saving cpu
      std::this_thread::yield();
      retry_times = 0;
    }
  }
  // 成功获取写锁，等待的写锁数减 1
  write_lock_wait_num_.fetch_sub(1);
}

inline void AtomicRWLock::ReadUnlock() { lock_num_.fetch_sub(1); }

inline void AtomicRWLock::WriteUnlock() { lock_num_.fetch_add(1); }

} // namespace base
} // namespace cmw
} // namespace hnu

#endif