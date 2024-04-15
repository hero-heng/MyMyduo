#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>

#include <string>
#include <functional>

class Server
{
public:
    Server(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : loop_(loop),
          server_(loop, addr, name)
    {
        // 注册回调函数
        server_.setConnectionCallback(
            std::bind(&Server::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(
            std::bind(&Server::onMessage, this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));
        // 设置合适的loop线程数量
        server_.setThreadNum(3);
    }
    void start()
    {
        server_.start();
    }

private:
    // 连接建立或者断开时的回调
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown(); // 关闭写端 EPOLLHUP
    }

    TcpServer server_;
    EventLoop *loop_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);

    Server server(&loop, addr, "server-01");
    server.start();
    loop.loop();

    return 0;
}