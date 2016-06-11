#pragma once

#include "Module.h"

struct PlayersModuleData;

const i32 UNASSIGNED = -1;

enum ShipType
{
    Ship_Warbird = 0,
    Ship_Javelin = 1,
    Ship_Spider = 2,
    Ship_Leviathan = 3,
    Ship_Terrier = 4,
    Ship_Weasel = 5,
    Ship_Lancaster = 6,
    Ship_Shark = 7,
    Ship_Spec = 8,
};

struct PlayerData
{
    string name = "unassigned name";
    string squad = "unassigned squad";

    i32 pid = UNASSIGNED;
    i32 freq = UNASSIGNED;
    ShipType ship = Ship_Spec;
};

class Players : public Module
{
   public:
    Players(Client& c);

    shared_ptr<PlayerData> GetPlayerData(i32 pid);
    void UpdatePlayerList();

   private:
    shared_ptr<PlayersModuleData> data;
};
