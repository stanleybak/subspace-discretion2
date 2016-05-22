/*
 * Config.h
 *
 *  Created on: May 21, 2016
 *      Author: stan
 */

#include "Client.h"
#include <memory>
#include "../lib/inih/INIReader.h"
using namespace std;

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

class Config : public Module
{
    using Module::Module;  // use Module's constructor

   public:
    void ReadConfig();

    const char* Get(const char* section, const char* name, const char* default_value);

    i32 GetInt(const char* section, const char* name, i32 default_value);

    double GetDouble(const char* section, const char* name, double default_value);

    bool GetBoolean(const char* section, const char* name, bool default_value);

   private:
    shared_ptr<INIReader> reader;
};

#endif  // SRC_CONFIG_H_
