#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWirteEvent = EPOLLOUT;

// EventLoop:ChannelList + Poller
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      tied_(false)

{
}

// channel的tie方法什么时候调用过？一个TcpConnection新连接创建的时候
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

// 在channel所属的EventLoop中，把当前的channel删除掉（一个EventLoop里面有一个channelList，把当前的channel从channelList中删除掉）
void Channel::remove()
{
    loop_->removeChannel(this);
}
/**
 * 当改变channel所表示的fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
 * EventLoop => channelList Poller 后面两个都是相互独立的
 */
void Channel::update()
{
    // 通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock(); // 使用lock方法将弱指针提升为强智能指针
        if (guard)
        { // 如果提升成功
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// EventLoop 调用 Poller 来监听事件,然后 Channel根据具体的事件来调用相应回调函数
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
            closeCallback_();
    }

    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
            errorCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
            readCallback_(receiveTime);
    }
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
            writeCallback_();
    }
}
