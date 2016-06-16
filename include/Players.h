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

struct PlayerPhysics
{
    i32 x = 8192 * 10000;  // pixel * 10000
    i32 y = 8192 * 10000;
    i32 xvel = 0;  // pixel * 10000 per second
    i32 yvel = 0;

    i32 rot = 0;  // rotation; degrees * 10000
};

struct Player
{
    string name = "unassigned name";
    string squad = "unassigned squad";

    i32 pid = UNASSIGNED;
    i32 freq = UNASSIGNED;
    ShipType ship = Ship_Spec;

    PlayerPhysics physics;

    i32 GetXPixel();
    i32 GetYPixel();
    i32 GetXTile();
    i32 GetYTile();
    i32 GetRotFrame();

    void SetPixel(i32 x, i32 y);
};

class Players : public Module
{
   public:
    Players(Client& c);

    shared_ptr<Player> GetPlayer(i32 pid);
    shared_ptr<Player> GetSelfPlayer(bool logErrors = true);

    void UpdatePlayerList();

   private:
    shared_ptr<PlayersModuleData> data;
};
