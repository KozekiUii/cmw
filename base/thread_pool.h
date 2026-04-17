#ifndef CMW_BASE_THREAD_POOL_H_
#define CMW_BASE_THREAD_POOL_H_


#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <cmw/base/bounded_queue.h>
namespace hnu    {
namespace cmw   {
namespace base {

class ThreadPool {
 public:
  explicit ThreadPool(std::size_t thread_num, std::size_t max_task_num = 1000);

  template <typename F, typename... Args>
  auto Enqueue(F&& f, Args&&... args)
      -> std::future<typename std::result_of<F(Args...)>::type>;

  ~ThreadPool();

 private:
  BoundedQueue<std::function<void()>> task_queue_;
  std::vector<std::thread> workers_;
  std::atomic_bool stop_;
};

/**
 * @brief 构造函数，创建线程池并初始化任务队列
 *
 * @param threads
 * @param max_task_num
 */
inline ThreadPool::ThreadPool(std::size_t threads, std::size_t max_task_num)
    : stop_(false) {
  /*创建一个BoundedQueue，采用的等待策略是阻塞策略*/
  if (!task_queue_.Init(max_task_num, new BlockWaitStrategy())) {
    throw std::runtime_error("Task queue init failed.");
  }

  /* 初始化线程池 创建空的任务，每个任务都是一个while循环 */
  // reserve预分配内存,避免频繁的内存分配
  workers_.reserve(threads);
  // 线程池中的线程循环等待领取任务并执行
  for (size_t i = 0; i < threads; ++i) {
    // 就地构造线程对象，lambda函数作为线程的执行体
    workers_.emplace_back([this] {
      while (!stop_) {
        /*返回值为空的可调用对象*/
        std::function<void()> task;
        if (task_queue_.WaitDequeue(&task)) {
          /*如果出队成功，说明领取到了任务，则就去执行此任务*/
          task();
        }
      }
    });
  }
}

// before using the return value, you should check value.valid()


/**
 * @brief 线程池任务入队，接受一个可调用对象f和任意数量的参数args...，并返回一个std::future，其类型是调用f(args...)后的返回类型。
 *
 * @tparam F
 * @tparam Args
 * @param f
 * @param args
 * @return std::future<typename std::result_of<F(Args...)>::type>
 */
template <typename F, typename... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
  using return_type = typename std::result_of<F(Args...)>::type;

  // bind packages the function and parameters into a parameterless call function return_type(), packaged_task wraps a parameterless function with a return value of return_type, which can be obtained through future, and finally make_shared hands over control to shared_ptr
  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
  // get the future of the task
  std::future<return_type> res = task->get_future();

  // don't allow enqueueing after stopping the pool
  if (stop_) {
    return std::future<return_type>();
  }
  // lambda function: capture task, () is a parameterless function, (*task)() is a calling task
  task_queue_.Enqueue([task]() { (*task)(); });
  return res;
};

// the destructor joins all threads
/* 唤醒线程池里所有线程，然后等待所有子线程执行完毕，释放资源*/
inline ThreadPool::~ThreadPool() {
  // prevent double destruction
  if (stop_.exchange(true)) {
    return;
  }
  // wake up all threads
  task_queue_.BreakAllWait();
  // wait for all threads to finish
  for (std::thread& worker : workers_) {
    // join() blocks the calling thread until the thread it is called on terminates
    worker.join();
  }
}



}
}
}

#endif
