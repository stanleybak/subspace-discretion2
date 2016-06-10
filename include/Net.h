/*
 * Net.h
 *
 *  Created on: May 30, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"
#include "Packets.h"

const u8 RELIABLE_HEADER = 0x03;
const u8 CORE_HEADER = 0x00;
const u8 CLUSTER_HEADER = 0x0E;

struct NetData;

class Net : public Module
{
   public:
    Net(Client& c);
    ~Net();

    bool NewConnection(const char* hostname, u16 port);
    void DisconnectSocket();

    void AddPacketHandler(const char* name, std::function<void(const PacketInstance*)> func);
    void SendPacket(PacketInstance* packet);
    void SendReliablePacket(PacketInstance* packet);

    // periodically called
    void ReceivePackets(i32 ms);
    void SendPackets(i32 ms);

   private:
    shared_ptr<NetData> data;
};
