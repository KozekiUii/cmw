#ifndef CMW_COMMON_UTIL_H_
#define CMW_COMMON_UTIL_H_


#include <string>
#include <type_traits>


namespace hnu {
namespace cmw {
namespace common {

/*
  std::hash 是一个函数对象模板，提供了对各种类型进行哈希计算的功能。std::hash<std::string> 是 std::hash 模板的一个特化版本，用于计算 std::string 类型的哈希值
*/
inline std::size_t Hash(const std::string &key) {
  return std::hash<std::string>{}(key);
}



}
}
}



#endif