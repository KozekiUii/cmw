#include "../../bounded_queue.h"
#include <iostream>
#include <ostream>

using namespace hnu::cmw::base;

int main(int argc, char *argv[]) {

  std::cout << "---------------->start" << std::endl;
  BoundedQueue<int> queue;
  queue.Init(3, new TimeoutBlockWaitStrategy(1000));
  bool b = queue.WaitEnqueue(1);
  std::cout << b << std::endl;
  b = queue.WaitEnqueue(2);
  std::cout << b << std::endl;
  b = queue.WaitEnqueue(3);
  std::cout << b << std::endl;
  b = queue.WaitEnqueue(4);
  std::cout << b << std::endl;
  // 这里不会被执行
  // int element;
  // b = queue.Dequeue(&element);
  // std::cout << element << std::endl;

  std::cout << "---------------->end" << std::endl;
  return 0;
}