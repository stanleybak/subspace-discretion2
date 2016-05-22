/*
 * Config.h
 *
 *  Created on: May 21, 2016
 *      Author: stan
 */
#pragma once

#include "Module.h"
#include "../lib/inih/INIReader.h"

class Config : public Module
{
   public:
    Config(Client& c);

    const char* GetString(const char* section, const char* name, const char* default_value);
    const char* GetString(const char* section, const char* name, const char* def, bool (*IsAllowed)(const char*));

    i32 GetInt(const char* section, const char* name, i32 default_value);
    i32 GetInt(const char* section, const char* name, i32 default_value, bool (*IsAllowed)(i32));

    double GetDouble(const char* section, const char* name, double default_value);
    double GetDouble(const char* section, const char* name, double def, bool (*IsAllowed)(double));

    bool GetBoolean(const char* section, const char* name, bool default_value);

   private:
    INIReader reader;
};
