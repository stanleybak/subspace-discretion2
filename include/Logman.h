/*
 * Logman.h
 *
 *  Created on: May 21, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"
#include <stdarg.h>

enum LogLevel
{
    LOG_DRIVEL,
    LOG_INFO,
    LOG_ERROR,
};

class Logman : public Module
{
   public:
    Logman(Client& c);
    ~Logman();

    void LogError(const char* format, ...);
    void LogInfo(const char* format, ...);
    void LogDrivel(const char* format, ...);
    void Log(LogLevel l, const char* format, ...);

    // log an error and then exit
    void FatalError(const char* format, ...);

    LogLevel logLevelSave = LOG_INFO;         // minimum log level for saving to the terminal
    LogLevel logLevelPrint = LOG_DRIVEL;      // minimum log level for printing
    LogLevel logLevelPrintStderr = LOG_INFO;  // minimum log level for printing to stderr

   private:
    FILE* f = nullptr;
    void LogHeader();
    void LogVaList(LogLevel l, const char* format, va_list args);
};
