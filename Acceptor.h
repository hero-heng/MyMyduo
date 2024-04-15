#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor
{
public:
    using NewConnectCallback = std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reusePort);
    ~Acceptor();

    void setNewConnectCallback(const NewConnectCallback &cb)
    {
        newConnectCallback_ = cb;
    }

    bool listenning() const { return listenning_; }
    void listen();

private:
    void handleRead();

    EventLoop *loop_; // Acceptor用的就是用户定义的那个baseloop，也称作mainloop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectCallback newConnectCallback_;
    bool listenning_;
};
