/*
 * Connection.h
 *
 *  Created on: Jun 4, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"

struct ConnectionData;

class Connection : public Module
{
   public:
    Connection(Client& c);
    ~Connection();

    void Connect(const char* name, const char* pw, const char* hostname, u16 port);

   private:
    shared_ptr<ConnectionData> data;
};
