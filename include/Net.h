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

    const char* GetPlayerName();

    bool NewConnection(const char* hostname, u16 port);
    void DisconnectSocket();

    void AddPacketHandler(const char* name, std::function<void(const PacketInstance*)> func);
    void SendPacket(PacketInstance* packet, const char* templateName);
    void SendReliablePacket(PacketInstance* packet, const char* templateName);

    // periodically called
    void ReceivePackets(i32 ms);
    void SendPackets(i32 ms);

   private:
    shared_ptr<NetData> data;
};
