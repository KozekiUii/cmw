#ifndef CMW_TRANSPORT_SHM_BLOCK_H_
#define CMW_TRANSPORT_SHM_BLOCK_H_


#include <atomic>
#include <cstdint>

namespace hnu{
namespace cmw{
namespace transport{


class Block
{
    friend class Segment;
public:
    Block();
    virtual ~Block();

    uint64_t msg_size() const { return msg_size_; }
    void set_msg_size(uint64_t msg_size) { msg_size_ = msg_size;}
    uint64_t msg_info_size() const { return msg_info_size_; }
    void set_msg_info_size(uint64_t msg_info_size) {
        msg_info_size_ = msg_info_size;
    }

    static const int32_t kRWLockFree;       // 读写锁空闲
    static const int32_t kWriteExclusive;
    static const int32_t kMaxTryLockTimes;    // 最大尝试加锁次数
    
private:
    // 尝试获取写锁（独占锁）
    bool TryLockForWrite();
    // 尝试获取读锁（共享锁）
    bool TryLockForRead();
    // 释放写锁
    void ReleaseWriteLock();
    // 释放读锁
    void ReleaseReadLock();

    std::atomic<int32_t> lock_num_ = {0};   // 读写锁数量
    uint64_t msg_size_; // 消息大小
    uint64_t msg_info_size_; // 消息信息大小
};



}
}
}

#endif