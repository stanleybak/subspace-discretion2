#include "Players.h"
#include "Net.h"
#include "Graphics.h"
#include "Ships.h"
#include <map>
#include <set>
using namespace std;

struct PlayersModuleData
{
    Client& c;

    shared_ptr<Player> selfData = make_shared<Player>();
    map<i32, shared_ptr<Player>> idToPlayerMap;

    vector<shared_ptr<DrawnText>> displayedNames;

    map<ShipType, string> SHIP_NAMES = {
        {Ship_Warbird, "Warbird"},
        {Ship_Javelin, "Javelin"},
        {Ship_Spider, "Spider"},
        {Ship_Leviathan, "Leviathan"},
        {Ship_Terrier, "Terrier"},
        {Ship_Weasel, "Weasel"},
        {Ship_Lancaster, "Lancaster"},
        {Ship_Shark, "Shark"},
        {Ship_Spec, "Spectator"},
    };

    PlayersModuleData(Client& c) : c(c) {}

    void UpdatePlayerList()
    {
        char buf[128];

        // first add the player
        snprintf(buf, sizeof(buf), "%s [%s] (%i)", selfData->name.c_str(),
                 SHIP_NAMES[selfData->ship].c_str(), selfData->freq);
        string selfName = buf;

        // then add all his teammates alphabetically
        set<string> teammates;

        for (auto i : idToPlayerMap)
        {
            if (i.second->pid == selfData->pid)
                continue;

            if (i.second->freq != selfData->freq)
                continue;

            snprintf(buf, sizeof(buf), "%s [%s]", i.second->name.c_str(),
                     SHIP_NAMES[i.second->ship].c_str());
            teammates.insert(buf);
        }

        // then add everyone else alphabetically
        set<string> enemies;

        for (auto i : idToPlayerMap)
        {
            if (i.second->freq == selfData->freq)
                continue;

            snprintf(buf, sizeof(buf), "%s [%s]", i.second->name.c_str(),
                     SHIP_NAMES[i.second->ship].c_str());
            enemies.insert(buf);
        }

        // make the DrawnText objects
        i32 x = 10;
        i32 y = 35;

        displayedNames.clear();

        shared_ptr<DrawnText> text =
            c.graphics->MakeDrawnText(Layer_Gauges, Color_Yellow, selfName.c_str());
        text->SetPosition(x, y);
        y += c.graphics->GetFontHeight();
        displayedNames.push_back(text);

        for (const string& s : teammates)
        {
            text = c.graphics->MakeDrawnText(Layer_Gauges, Color_Yellow, s.c_str());
            text->SetPosition(x, y);
            y += c.graphics->GetFontHeight();
            displayedNames.push_back(text);
        }

        for (const string& s : enemies)
        {
            text = c.graphics->MakeDrawnText(Layer_Gauges, Color_Yellow, s.c_str());
            text->SetPosition(x, y);
            y += c.graphics->GetFontHeight();
            displayedNames.push_back(text);
        }
    }

    std::function<void(const PacketInstance*)> handlePidChange = [this](const PacketInstance* pi)
    {
        selfData->pid = pi->GetIntValue("pid");

        c.log->LogDrivel("Got Self PID change to %i", selfData->pid);
    };

    std::function<void(const PacketInstance*)> playerLeaving = [this](const PacketInstance* pi)
    {
        i32 pid = pi->GetIntValue("pid");

        idToPlayerMap.erase(pid);
        c.players->UpdatePlayerList();

        c.log->LogDrivel("Player Left (pid=%i)", pid);
    };

    std::function<void(const PacketInstance*)> playerEntering = [this](const PacketInstance* pi)
    {
        shared_ptr<Player> player;
        i32 pid = pi->GetIntValue("pid");

        if (pid == selfData->pid)
            player = selfData;
        else
            player = make_shared<Player>();

        player->name = *pi->GetStringValue("name");
        player->squad = *pi->GetStringValue("squad");
        player->freq = pi->GetIntValue("freq");
        player->pid = pid;
        ShipType ship = Ship_Spec;

        i32 shipNum = pi->GetIntValue("ship");

        if (shipNum >= 0 && shipNum <= 8)
            ship = (ShipType)shipNum;

        player->ship = ship;

        idToPlayerMap[player->pid] = player;

        c.players->UpdatePlayerList();

        if (player == selfData)
            c.ships->ShipChanged(ship);  // handle an initial ship changed event

        c.log->LogDrivel("Player Entering (pid=%i,name=%s)", player->pid, player->name.c_str());
    };

    std::function<void(const PacketInstance*)> freqChange = [this](const PacketInstance* pi)
    {
        i32 pid = pi->GetIntValue("pid");
        i32 freq = pi->GetIntValue("freq");

        auto it = idToPlayerMap.find(pid);

        if (it == idToPlayerMap.end())
            c.log->LogError("Freq Change for unknown pid %d", pid);
        else
        {
            it->second->freq = freq;

            c.players->UpdatePlayerList();

            c.log->LogDrivel("Freq Change to %i (%s)", freq, it->second->name.c_str());
        }
    };

    std::function<void(const PacketInstance*)> freqShipChanged = [this](const PacketInstance* pi)
    {
        i32 pid = pi->GetIntValue("pid");
        i32 freq = pi->GetIntValue("freq");
        ShipType ship = Ship_Spec;

        i32 shipNum = pi->GetIntValue("ship");

        if (shipNum >= 0 && shipNum <= 8)
            ship = (ShipType)shipNum;

        auto it = idToPlayerMap.find(pid);

        if (it == idToPlayerMap.end())
            c.log->LogError("Freq Ship Change for unknown pid %d", pid);
        else
        {
            shared_ptr<Player> player = it->second;

            player->freq = freq;
            player->ship = ship;

            c.players->UpdatePlayerList();

            if (player == selfData)
            {
                player->SetPixel(8192, 8192);

                c.ships->ShipChanged(ship);
            }

            c.log->LogDrivel("FreqShip Change to freq=%i, ship=%i (%s)", freq, shipNum,
                             player->name.c_str());
        }
    };
};

Players::Players(Client& c) : Module(c), data(make_shared<PlayersModuleData>(c))
{
    c.net->AddPacketHandler("pid change", data->handlePidChange);

    c.net->AddPacketHandler("player leaving", data->playerLeaving);
    c.net->AddPacketHandler("player entering", data->playerEntering);
    c.net->AddPacketHandler("freq change", data->freqChange);
    c.net->AddPacketHandler("freq ship changed", data->freqShipChanged);
}

shared_ptr<Player> Players::GetPlayer(i32 pid)
{
    shared_ptr<Player> rv = nullptr;
    auto it = data->idToPlayerMap.find(pid);

    if (it == data->idToPlayerMap.end())
        rv = nullptr;
    else
        rv = it->second;

    if (rv == nullptr)
        c.log->LogError("Players::GetPlayer(id) failed to find player with pid %i", pid);

    return rv;
}

shared_ptr<Player> Players::GetSelfPlayer(bool logErrors)
{
    if (logErrors && data->selfData == nullptr)
        c.log->LogError("Players::GetSelfPlayer(id) returning nullptr");

    return data->selfData;
}

void Players::UpdatePlayerList()
{
    data->UpdatePlayerList();
}

i32 Player::GetXPixel()
{
    return physics.x / 10000;
}

i32 Player::GetRotFrame()
{
    const int NUM_FRAMES = 40;
    const int FULL_ROTATION = (360 * 10000);

    return (physics.rot * NUM_FRAMES) / FULL_ROTATION;
}

void Player::SetPixel(i32 x, i32 y)
{
    physics.x = 10000 * x;
    physics.y = 10000 * y;
}

i32 Player::GetYPixel()
{
    return physics.y / 10000;
}

i32 Player::GetXTile()
{
    return physics.x / 10000 / 16;
}

i32 Player::GetYTile()
{
    return physics.y / 10000 / 16;
}
