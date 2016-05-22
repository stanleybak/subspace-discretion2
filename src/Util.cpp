#include "Util.h"

#ifndef WIN32
// posix
#include <sys/time.h>
#else
// windows
#include <windows.h>
#endif  // WIN32

#include <stdio.h>
#include <string.h>

i32 Util::getWallMills()
{
    i32 rv;
#ifdef WIN32
    SYSTEMTIME time;
    GetSystemTime(&time);
    rv = (time.wSecond * 1000) + time.wMilliseconds;
#else
    // posix!
    struct timeval tv;
    // get the time this was received
    if (gettimeofday(&tv, 0) < 0)
    {
        // error unable to get clocktime
        c.log->LogError("error in system call gettimeofday: %s", strerror(errno));
        rv = 0;
    }
    else
    {
        rv = (int)(tv.tv_sec * 1000) + (int)(tv.tv_usec / 1000);
    }
#endif

    return rv;
}
