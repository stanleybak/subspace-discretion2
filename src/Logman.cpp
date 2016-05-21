#include "Logman.h"
#include <stdio.h>
#include <map>
using std::map;

static map<LogLevel, const char*> LOG_PREFIX = {
    {LOG_DRIVEL, "Drivel"}, {LOG_INFO, "Info"}, {LOG_ERROR, "Error"},
};

Logman::Logman(Client& client) : Module(client)
{
}

void Logman::LogError(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    LogVaList(LOG_ERROR, format, args);

    va_end(args);
}

void Logman::LogInfo(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    LogVaList(LOG_INFO, format, args);

    va_end(args);
}

void Logman::LogDrivel(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    LogVaList(LOG_DRIVEL, format, args);

    va_end(args);
}

void Logman::Log(LogLevel l, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    LogVaList(l, format, args);

    va_end(args);
}

void Logman::FatalError(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    LogVaList(LOG_ERROR, format, args);

    va_end(args);

    // exit?
    c.sdl.Exit();
}

void Logman::LogVaList(LogLevel level, const char* format, va_list args)
{
    printf("%s: ", LOG_PREFIX[level]);
    vprintf(format, args);

    if (level >= LOG_ERROR)
        fflush(stdout);
}
