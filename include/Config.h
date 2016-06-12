/*
 * Config.h
 *
 *  Created on: May 21, 2016
 *      Author: stan
 */
#pragma once

#include "Module.h"
#include <map>
#include <vector>

class Config : public Module
{
   public:
    Config(Client& c);

    const char* GetString(const char* section, const char* name, const char* default_value);
    const char* GetString(const char* section, const char* name, const char* def,
                          bool (*IsAllowed)(const char*));

    i32 GetInt(const char* section, const char* name, i32 default_value);
    i32 GetInt(const char* section, const char* name, i32 default_value, bool (*IsAllowed)(i32));

    double GetDouble(const char* section, const char* name, double default_value);
    double GetDouble(const char* section, const char* name, double def, bool (*IsAllowed)(double));

    vector<i32> GetIntList(const char* section, const char* name);

    // may return a null pointer; doesn't use c.log on missing values
    const char* GetStringNoDefault(const char* section, const char* name);

   private:
    void LoadSettings(const char* path);
    map<string, map<string, string> > settingsMap;
};
