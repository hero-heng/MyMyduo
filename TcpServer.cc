#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <strings.h>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainloop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

/**
 * 创建TcpServer对象
 *
 *      创建acceptor对象
*            =》1.创建一个socket（listenfd） ,封装到一个acceptChannel中
*               2.将acceptChannel添加到当前loop中的poller里面，注册一个读事件
                3.然后poller开始监听acceptChannel上的事件，事件发生后，Poller执行一个读事件的回调（绑定的是Acceptor::handleRead
                4.Acceptor::handleRead里面通过accept函数返回一个connfd（跟客户端通信的connfd
                5.并执行新连接的回调 newConnectionCallback_(connfd,peerAddr);  (这个回调是通过setNewConnectionCallback来设置的)
                                    =>这个函数实际上执行的是TcpServer::newConnection
                                        这个函数做的任务是：
                                        1.根据轮询算法选择一个subloop
                                        2.唤醒subloop
                                        3.把当前connfd封装成channel分发给subloop
                                        要注意的是：运行在主线程中
                                                    主线程的mianloop调用newConnection,选择了一个ioLoop
*/
TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string &nameArg,
                     Option option)
    : loop_(CheckLoopNotNull(loop)),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop_, name_)),
      connecttionCallback_(),
      messageCallback_(),
      nextConnId_(1),
      started_(0)
{
    // 当有新用户连接时，会执行TcpServer::newConnection回调
    acceptor_->setNewConnectCallback(std::bind(&TcpServer::newConnection, this,
                                               std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second); // 这个局部的shared_ptr智能指针对象(new出来的），出括号后，可以自动释放new出来的TcpConnection对象资源了
        item.second.reset(); //原来的指针被释放

        //销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numthreads)
{
    threadPool_->setThreadNum(numthreads);
}

// 开启服务器监听后，就是执行loop.loop()
void TcpServer::start()
{
    if (started_++ == 0) // 防止一个TcpServer对象被start多次,一开始started_变量默认设置成零，start一次之后就无法进入这个if
    {
        threadPool_->start(threadInitCallback_); // 启动底层的loop线程池
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// 有一个新的客户端连接，acceptor会执行这个回调操作
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // 轮询算法，来选择一个subloop，来管理channel
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr errorn\n");
    }
    InetAddress localAddr(local);

    // 根据连接成功的sockfd，创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(
        ioLoop,
        connName,
        sockfd,
        localAddr,
        peerAddr));
    connections_[connName] = conn;
    // 下面的回调都是用户设置给TcpServer的，然后Tcpserver=》TcpConnection=>Channel=>Poller=>notify Channel调用回调
    conn->setConnectionCallback(connecttionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    // 设置了如何关闭连接的回调
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // 直接调用TcpConnection::connectEstablished
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
             name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}
