/*
 * Net.h
 *
 *  Created on: May 30, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"

struct NetData;

class Net : public Module
{
   public:
    Net(Client& c);
    ~Net();

    const char* GetPlayerName();

   private:
    shared_ptr<NetData> data;
};
