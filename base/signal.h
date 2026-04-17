#ifndef CMW_BASE_SIGNAL_H_
#define CMW_BASE_SIGNAL_H_

#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <mutex>


namespace hnu    {
namespace cmw   {
namespace base {

template <typename... Args>
class Slot;

template <typename... Args>
class Connection;

/*被观察者
  SlotList成员，记录了关联在该信号下的所有槽
*/
template <typename... Args>
class Signal {
 public:
  using Callback = std::function<void(Args...)>;
  using SlotPtr = std::shared_ptr<Slot<Args...>>;
  using SlotList = std::list<SlotPtr>;
  using ConnectionType = Connection<Args...>;

  Signal() {}
  virtual ~Signal() { DisconnectAllSlots(); }

  //重载了()操作符，也就是说当像这样调用时 signal(msg, msg_info)，
  //就会对该信号对应的所有槽（所有关联的回调函数）进行一次调用，这其实就是通知所有监听该信号的回调函数。
  void operator()(Args... args) {
    SlotList local;
    // 锁内复制槽列表，避免在调用槽函数时持有锁，导致死锁
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (auto& slot : slots_) {
        local.emplace_back(slot);
      }
    }

    // 锁外调用槽函数，避免死锁
    if (!local.empty()) {
      for (auto& slot : local) {
        (*slot)(args...);
      }
    }

    ClearDisconnectedSlots();
  }

  //为某个回调函数创建一个Slot共享指针，然后加入到自己的槽列表并返回一个Connection关联实例
  ConnectionType Connect(const Callback &cb) {
    // 创建一个新的Slot实例，并将其封装在一个shared_ptr中
    auto slot = std::make_shared<Slot<Args...>>(cb);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      slots_.emplace_back(slot);
    }

    return ConnectionType(slot, this);
  }

  //接收一个Connection参数，从槽列表中找到该槽，然后将槽的标记置为false并从列表中删除。
  bool Disconnect(const ConnectionType& conn) {
    bool find = false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (auto& slot : slots_) {
        if (conn.HasSlot(slot)) {
          find = true;
          slot->Disconnect();
        }
      }
    }

    if (find) {
      ClearDisconnectedSlots();
    }
    return find;
  }

  void DisconnectAllSlots() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& slot : slots_) {
      slot->Disconnect();
    }
    // list.clear()会调用每个元素的析构函数，shared_ptr的析构函数会自动释放资源
    slots_.clear();
  }

 private:
  Signal(const Signal&) = delete;
  Signal& operator=(const Signal&) = delete;

  void ClearDisconnectedSlots() {
    std::lock_guard<std::mutex> lock(mutex_);
    // 移除所有已断开连接的槽
    // 先使用std::remove_if将不满足条件的元素移到末尾，然后使用erase删除这些元素
    slots_.erase(
        std::remove_if(slots_.begin(), slots_.end(),
                       [](const SlotPtr& slot) { return !slot->connected(); }),
        slots_.end());
  }

  SlotList slots_;
  std::mutex mutex_;
};


/**
 * @brief 连接器：
  保存了一个Slot的shared_ptr和一个指向Signal的指针，提供了Disconnect函数用来断开连接。
   Connection类的实例在Signal::Connect函数中被创建并返回给调用者，调用者可以通过这个实例来管理连接，比如断开连接等。
 * @union slot_: 保存了一个Slot的shared_ptr，表示与Signal关联的槽。
 * @union signal_: 保存了一个指向Signal的指针，表示该连接所属的信号。
 *
 * @tparam Args
 */
template <typename... Args>
class Connection {
 public:
  using SlotPtr = std::shared_ptr<Slot<Args...>>;
  using SignalPtr = Signal<Args...>*;

  Connection() : slot_(nullptr), signal_(nullptr) {}
  Connection(const SlotPtr& slot, const SignalPtr& signal)
      : slot_(slot), signal_(signal) {}
  virtual ~Connection() {
    slot_ = nullptr;
    signal_ = nullptr;
  }

  Connection& operator=(const Connection& another) {
    if (this != &another) {
      this->slot_ = another.slot_;
      this->signal_ = another.signal_;
    }
    return *this;
  }

  bool HasSlot(const SlotPtr& slot) const {
    if (slot != nullptr && slot_ != nullptr) {
      return slot_.get() == slot.get();
    }
    return false;
  }

  bool IsConnected() const {
    if (slot_) {
      return slot_->connected();
    }
    return false;
  }

  bool Disconnect() {
    if (signal_ && slot_) {
      return signal_->Disconnect(*this);
    }
    return false;
  }

 private:
  SlotPtr slot_;  // 保存了一个Slot的shared_ptr，表示与Signal关联的槽
  SignalPtr signal_;  // 保存了一个指向Signal的指针，表示该连接所属的信号
};

/*观察者:
  保存了一个回调函数std::function<void(Args...)> cb_和一个标记bool connected_，
  提供一个Disconnect函数用来将标记置为false。
  重载了()操作符，当被调用时就会去运行cb_函数。
*/
template <typename... Args>
class Slot {
 public:
  using Callback = std::function<void(Args...)>;
  Slot(const Slot& another)
      : cb_(another.cb_), connected_(another.connected_) {}
  explicit Slot(const Callback& cb, bool connected = true)
      : cb_(cb), connected_(connected) {}
  virtual ~Slot() {}

  void operator()(Args... args) {
    if (connected_ && cb_) {
      cb_(args...);
    }
  }

  void Disconnect() { connected_ = false; }
  bool connected() const { return connected_; }

 private:
  Callback cb_;
  bool connected_ = true;
};
}
}
}


#endif
