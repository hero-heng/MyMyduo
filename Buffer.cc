#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据 Poller工作在LT模式
 * Buffer缓冲区是有大小的！但是从fd上读取数据的时候，不知道Tcp数据最终的大小（读一段，放入缓冲区一段）
 */
ssize_t Buffer::readfd(int fd, int *saveErrno)
{
    char extraBuf[65536] = {0}; // 栈上内存空间 64k
    struct iovec vec[2];
    const size_t writable = writableBytes(); // 这是buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extraBuf;
    vec[1].iov_len = sizeof extraBuf;

    const int iovcnt = (writable < sizeof extraBuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable) // buffer可写缓冲区已经够写数据
    {
        writerIndex_ += n;
    }
    else // extrabuf里面也写了数据
    {
        writerIndex_ = buffer_.size();
        append(extraBuf, n - writable);
    }
    return n;
}

ssize_t Buffer::writefd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());

    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}
