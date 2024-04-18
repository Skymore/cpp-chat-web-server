#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>   // perror, 提供对错误处理的功能
#include <iostream>  // 输入输出流
#include <unistd.h>  // write, 提供基本的写入函数
#include <sys/uio.h> // readv, 提供多缓冲区的读取功能
#include <vector>    // 用于缓冲区的动态数组
#include <atomic>    // 提供原子操作，用于多线程中安全访问
#include <assert.h>  // 提供断言

class Buffer {
public:
    // 构造函数，指定初始缓冲区大小，默认为1024字节
    Buffer(int initBuffSize = 1024);

    // 默认析构函数
    ~Buffer() = default;

    // 可写字节数
    size_t WritableBytes() const;
    // 可读字节数
    size_t ReadableBytes() const;
    // 头部可用字节数，通常用于协议中的头部信息
    size_t PrependableBytes() const;

    // 获取当前读取位置的指针
    const char* Peek() const;
    // 确保有足够的写入空间，不足时进行扩展
    void EnsureWriteable(size_t len);
    // 更新写入位置
    void HasWritten(size_t len);

    // 从缓冲区中取出指定长度的数据，但不返回数据内容
    void Retrieve(size_t len);
    // 从缓冲区中取出直到某个指定位置的数据
    void RetrieveUntil(const char* end);

    // 清空整个缓冲区
    void RetrieveAll();
    // 取出整个缓冲区的内容，并转换为字符串
    std::string RetrieveAllToStr();

    // 返回一个常量指针，指向当前的写入位置
    const char* BeginWriteConst() const;
    // 返回一个可修改的指针，指向当前的写入位置
    char* BeginWrite();

    // 向缓冲区追加数据
    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);

    // 从文件描述符读取数据到缓冲区中
    ssize_t ReadFd(int fd, int* Errno);
    // 将缓冲区的数据写入到文件描述符中
    ssize_t WriteFd(int fd, int* Errno);

private:
    // 获取缓冲区开始的指针（可修改和常量版本）
    char* BeginPtr_();
    const char* BeginPtr_() const;
    // 当需要时，扩展缓冲区空间
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;             // 实际存储数据的动态数组
    std::atomic<std::size_t> readPos_;     // 读取位置的原子变量
    std::atomic<std::size_t> writePos_;    // 写入位置的原子变量
};

#endif //BUFFER_H
