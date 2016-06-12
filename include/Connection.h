/*
 * Connection.h
 *
 *  Created on: Jun 4, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"
#include "Players.h"

struct ConnectionData;

// make to an acceptable, safe filename
string SanitizeString(const char* input);

enum ArenaType
{
    Arena_AnyPub,
    Arena_SpecificPub,
    Arena_Named_Arena,
};

class Connection : public Module
{
   public:
    Connection(Client& c);
    ~Connection();

    const char* GetPlayerName();
    void Connect(const char* name, const char* pw, const char* zonename, const char* hostname,
                 u16 port);
    void Disconnect();
    void UpdateConnectionStatus(i32 mills);

    bool isCompletelyDisconnected();
    bool isCompletelyConnected();

    void SendArenaLogin(ShipType s, const char* arenaName);
    void SendArenaLoginAnyPub(ShipType s);
    void SendArenaLoginSpecificPub(ShipType s, u16 num);

    string GetZoneDir();   // ends in '/' will mkdirs if it doesn't exist
    string GetArenaDir();  // ends in '/' will mkdirs if it doesn't exist

   private:
    shared_ptr<ConnectionData> data;
};
