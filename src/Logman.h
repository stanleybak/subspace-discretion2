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
   public:
    Logman(Client& client);

    void LogError(const char* format, ...);
    void LogInfo(const char* format, ...);
    void LogDrivel(const char* format, ...);
    void Log(LogLevel l, const char* format, ...);

    // log an error and then exit
    void FatalError(const char* format, ...);

   private:
    void LogVaList(LogLevel l, const char* format, va_list args);
};

#endif  // SRC_LOGMAN_H_
