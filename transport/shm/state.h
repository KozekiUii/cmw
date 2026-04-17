#ifndef CMW_TRANSPORT_SHM_STATE_H_
#define CMW_TRANSPORT_SHM_STATE_H_


#include <atomic>
#include <cstring>
#include <mutex>

namespace hnu{
namespace cmw{
namespace transport{

class State
{

public:
    explicit State(const uint64_t& ceiling_msg_size);
    virtual ~State();

    // 减少引用计数（有边界检查）
    void DecreaseReferenceCounts() {
      uint32_t current_reference_count = reference_count_.load();
      // CAS
        do{
            if(current_reference_count == 0)
            {
                return;
            }
        } while (!reference_count_.compare_exchange_weak(
            current_reference_count,        // 期望值
            current_reference_count - 1,    // 新值
            std::memory_order_acq_rel,      // 成功时的内存序
            std::memory_order_relaxed));    // 失败时的内存序
 
    }

    // 增加引用计数（无边界检查）
    void IncreaseReferenceCounts() { reference_count_.fetch_add(1); }

    // 增加序列号
    uint32_t FetchAddSeq(uint32_t diff) { return seq_.fetch_add(diff); }

    // 返回序列号
    uint32_t seq() { return seq_.load(); }

    // 设置是否需要重新映射共享内存
    void set_need_remap(bool need) { need_remap_.store(need);}
    // 返回是否需要重新映射共享内存
    bool need_remap() { return need_remap_;}
    // 返回消息上限大小
    uint64_t ceiling_msg_size() { return ceiling_msg_size_.load(); }
    // 返回引用计数
    uint32_t reference_counts() { return reference_count_.load(); }

    
private:
    std::atomic<bool> need_remap_ = {false};        // 标识是否需要重新映射共享内存
    std::atomic<uint32_t> seq_ = {0};               // 序列号，用于标识消息的顺序
    std::atomic<uint32_t> reference_count_ = {0};   // 引用计数器，跟踪有多少进程/线程在使用该共享内存段
    std::atomic<uint64_t> ceiling_msg_size_;        // 消息上限大小

};




}
}
}


#endif