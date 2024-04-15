#pragma once

#include <vector>
#include <string>
#include <algorithm>

/**
 * review mode
 * prependable bytes   |       readable bytes      |       writable bytes
 * 0                   readerIndex_                writerIndex_         size
 */
// 网络库底层的缓冲器类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;   // 前部分数据包的长度,也表示可读数据区的起始位置
    static const size_t kInitialSize = 1024; // 后部分缓冲区的大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + kInitialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend)
    {
    }
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char *peek() const { return begin() + readerIndex_; }

    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len; // 应用只读取了可读缓冲区的一部分长度（len）
        }
        else
        { // len == readableBytes()
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }
    // 调用onMessage时，把数据从buffer中取出来转换成c++ string格式
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); // 可读数据的长度
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // 上面一行代码把缓冲区中可读的数据，已经读取出来，这里肯定要对缓冲区进行复位操作（数据被取出来，下标要减少
        return result;
    }

    // 缓冲区可写的长度就是 buffer_size() - writerIndex
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); // 扩容函数
        }
    }

    //把[data,data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char *beginWrite() { return begin() + writerIndex_; }
    const char *beginWrite() const { return begin() + writerIndex_; }

    //从fd上读取数据
    ssize_t readfd(int fd,int *saveErrno);
    //从fd上发送数据
    ssize_t writefd(int fd,int *saveErrno);

private:
    char *begin() { return &*buffer_.begin(); } // 这里*buffer_.begin() 使用的是迭代器中的*运算重载函数，返回的是vector第一位元素，再&就是取地址
    const char *begin() const { return &*buffer_.begin(); }

    // 确保可写区的长度至少为len
    void makeSpace(size_t len)
    {
        /**
         * prepend bytes   |   reader  |   writer  |
         * ===>
         * prepend bytes   |             len            |
         *
         * 下面第一个式子是 可写区长度，第二个式子返回的是可读区的下标，
         * 二者相加就表示后面还剩write的长度加上前面prepend和有可能空出来的部分reader的总长度
         * 整体表达式的意思就是：前面已经空出来的区域大小加上后面可写的大小  还没有  我要求的可写区域 大
         * 所以需要resize 扩容
         */
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else // 相当于 用户已读的数据，reader空闲的大小加上可读的字节大小 能够满足len
        {
            size_t readable = readableBytes();
            // 把reader中后部分未读的数据，挪到reader的前面，使reader后部分跟writer连成一个整体
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};
