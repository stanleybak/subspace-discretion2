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

    const char* GetPlayerName();
    void Connect(const char* name, const char* pw, const char* hostname, u16 port);
    void Disconnect();
    void UpdateConnectionStatus(i32 mills);

    bool isCompletelyDisconnected();
    bool isCompletelyConnected();

   private:
    shared_ptr<ConnectionData> data;
};
