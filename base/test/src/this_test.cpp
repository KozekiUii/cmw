#include <iostream>

class Demo {
public:
    int a;    // 第一个成员
    int b;    // 第二个成员
    void printAddress() {
        // 打印 this 指针的值（对象的首地址）
        std::cout << "this 地址:   " << this << std::endl;
        // 打印第一个成员变量的地址
        std::cout << "变量 a 地址: " << &a << std::endl;
        // 打印第二个成员变量的地址
        std::cout << "变量 b 地址: " << &b << std::endl;
    }
};

int main() {
    Demo d1;
    d1.a = 10;
    d1.b = 20;
    d1.printAddress();
    return 0;
}