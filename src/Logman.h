/*
 * Logman.h
 *
 *  Created on: May 21, 2016
 *      Author: stan
 */

#include <stdarg.h>
#include "Client.h"

#ifndef SRC_LOGMAN_H_
#define SRC_LOGMAN_H_

enum LogLevel
{
    LOG_DRIVEL,
    LOG_INFO,
    LOG_ERROR,
};

class Logman : public Module
{
    using Module::Module;  // use Module's constructor

   public:
    ~Logman();

    void Open();

    void LogError(const char* format, ...);
    void LogInfo(const char* format, ...);
    void LogDrivel(const char* format, ...);
    void Log(LogLevel l, const char* format, ...);

    // log an error and then exit
    void FatalError(const char* format, ...);

    LogLevel logLevelSave = LOG_INFO;     // minimum log level for saving to the terminal
    LogLevel logLevelPrint = LOG_DRIVEL;  // minimum log level for printing

   private:
    FILE* f = nullptr;
    void LogHeader();
    void LogVaList(LogLevel l, const char* format, va_list args);
};

#endif  // SRC_LOGMAN_H_
