#ifndef CMW_TRANSPORT_SHM_SEGMENT_H_
#define CMW_TRANSPORT_SHM_SEGMENT_H_


#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <cmw/transport/shm/block.h>
#include <cmw/transport/shm/shm_conf.h>
#include <cmw/transport/shm/state.h>


namespace hnu{
namespace cmw{
namespace transport{



class Segment;
using SegmentPtr = std::shared_ptr<Segment>;

//可写的块内存结构体
struct WritableBlock
{
    uint32_t index = 0;
    Block* block = nullptr;
    uint8_t* buf = nullptr;
};

using ReadableBlock = WritableBlock;

class Segment
{

public:
    explicit Segment(uint64_t channel_id);
    virtual ~Segment() {}

    bool AcquireBlockToWrite(std::size_t msg_size, WritableBlock* writable_block);
    void ReleaseWrittenBlock(const WritableBlock& writable_block);

    bool AcquireBlockToRead(ReadableBlock* readable_block);
    void ReleaseReadBlock(const ReadableBlock& readable_block);

protected:
    virtual bool Destroy();
    virtual void Reset() = 0;   // 重置共享内存
    virtual bool Remove() = 0;  // 删除共享内存
    virtual bool OpenOnly() = 0; // 只读打开共享内存
    virtual bool OpenOrCreate() = 0; // 创建或打开共享内存
    bool init_;     // 初始化标志
    ShmConf conf_;  // 共享内存配置
    uint64_t channel_id_; // 通道ID

    State* state_; // 状态管理
    Block* blocks_; // 块内存
    void* managed_shm_;
    std::mutex block_buf_lock_; // 块缓冲区锁
    std::unordered_map<uint32_t, uint8_t*> block_buf_addrs_;    // 块缓冲区地址映射

private:
    bool Remap();   // 重新映射共享内存
    bool Recreate(const uint64_t& msg_size); // 重新创建共享内存
    uint32_t GetNextWritableBlockIndex(); // 获取下一个可写 Block 的索引
};






}
}
}
#endif