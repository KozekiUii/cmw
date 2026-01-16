#include <atomic>
#include <iostream>
 
#include "../../unbounded_queue.h"
 
using namespace hnu::cmw::base;
 
std::atomic<int> g_int;
 
int main(int argc, char* argv[]) {
  UnboundedQueue<std::string> que;
  que.Enqueue("a");
  que.Enqueue("b");
  que.Enqueue("b");
  que.Enqueue("d");
//   que.PrintData();
  std::string t;
  que.Dequeue(&t);
  std::cout << t << std::endl;
//   que.PrintData();
 
  return 0;
}