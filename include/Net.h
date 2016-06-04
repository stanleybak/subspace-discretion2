/*
 * Net.h
 *
 *  Created on: May 30, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"
#include "Packets.h"

struct NetData;

class Net : public Module
{
   public:
    Net(Client& c);
    ~Net();

    void AddPacketHandler(const char* name, std::function<void(const PacketInstance*)> func);
    void SendPacket(PacketInstance* packet, const char* templateName);
    void SendReliablePacket(PacketInstance* packet, const char* templateName);

    const char* GetPlayerName();

    bool NewConnection(const char* hostname, u16 port);
    void Disconnect();

    // periodically called
    void SendAndReceive(i32 iterationMs);

   private:
    shared_ptr<NetData> data;
};
