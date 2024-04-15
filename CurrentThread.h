#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrendThread
{
    extern __thread int t_cachedTid; //这里就是相当于thread_local
    
    void cacheTid();

    inline int tid()
    {
        if(__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}