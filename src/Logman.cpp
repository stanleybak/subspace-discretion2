#include "Logman.h"
#include "Config.h"
#include <map>
#include <stdarg.h>
using std::map;

static map<LogLevel, const char*> LOG_PREFIX = {
    {LOG_DRIVEL, "[D]"}, {LOG_INFO, "[I]"}, {LOG_ERROR, "[ERROR]"},
};

Logman::Logman(Client& client) : Module(client)
{
    const char* logFilename = c.cfg->GetStringNoDefault("log", "filename");

    if (logFilename == nullptr)
        logFilename = "log.txt";

    f = fopen(logFilename, "w");

    if (!f)
        FatalError("Error Opening Log File: %s", logFilename);
    else
        fprintf(f, "\n");

    LogHeader();
}

Logman::~Logman()
{
    if (f)
    {
        fclose(f);
        f = nullptr;
    }
}

void Logman::LogHeader()
{
    char buf[128];
    time_t rawtime;
    struct tm timeinfo;

    time(&rawtime);
    localtime_r(&rawtime, &timeinfo);
    // Wednesday, March 19, 2014 01:06:18 PM
    strftime(buf, 80, "%A, %B %d, %Y %I:%M:%S %p", &timeinfo);

    LogDrivel("Log file opened on %s", buf);
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

    abort();
}

void Logman::LogVaList(LogLevel level, const char* format, va_list args)
{
    if (level >= logLevelPrint || level >= logLevelPrintStderr)
    {
        va_list args_copy;
        va_copy(args_copy, args);  // copy in case we're printing to both terminal and file

        if (level >= logLevelPrintStderr)
        {
            fprintf(stderr, "%s ", LOG_PREFIX[level]);
            vfprintf(stderr, format, args_copy);
            fprintf(stderr, "\n");
        }
        else
        {
            printf("%s ", LOG_PREFIX[level]);
            vprintf(format, args_copy);
            printf("\n");
        }

        va_end(args_copy);
    }

    if (level >= logLevelSave && f != nullptr)
    {
        fprintf(f, "%s ", LOG_PREFIX[level]);
        vfprintf(f, format, args);
        fprintf(f, "\n");
    }

    if (level >= LOG_ERROR)
    {
        fflush(stdout);

        if (f != nullptr)
            fflush(f);
    }
}
