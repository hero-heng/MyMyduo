#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// muduo库中多路事件分发器的核心I/O复用模块
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    //给所有的I/O复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    //判断参数channel是否在当前的poller当中
    bool hasChannel(Channel *channel) const;

    //Evevtloop可以通过该接口获取默认的I/O复用实现事件
    //这块实现放在了defalutPoller文件里，因为思想是派生类可以调用基类，但是基类最好不能调用派生类
    static Poller* newDefaultPoller(EventLoop *loop);
protected:
    //map的key：sockfd value：sockfd所属的channel通道
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop* ownerLoop_;  //定义Poller所属的事件循环eventLoop
};
