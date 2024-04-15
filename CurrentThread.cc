#include "CurrentThread.h"

namespace CurrendThread
{
    __thread int t_cachedTid = 0;

    void cacheTid()
    {
        if(t_cachedTid == 0)
        {
            //通过linux系统调用获取当前线程tid
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }

}