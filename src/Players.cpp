#include "Players.h"
#include "Net.h"
#include "Graphics.h"
#include <map>
#include <set>
using namespace std;

struct PlayersModuleData
{
    Client& c;

    shared_ptr<PlayerData> selfData = make_shared<PlayerData>();
    map<i32, shared_ptr<PlayerData>> idToPlayerMap;

    vector<shared_ptr<DrawnText>> displayedNames;

    map<ShipType, string> SHIP_NAMES = {
        {Ship_Warbird, "Warbird"},
        {Ship_Javelin, "Javelin"},
        {Ship_Spider, "Spider"},
        {Ship_Leviathan, "Leviathan"},
        {Ship_Terrier, "Terrier"},
        {Ship_Weasel, "Weasel"},
        {Ship_Lancaster, "Lancaster"},
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

    std::function<void(PacketInstance*)> handlePidChange = [this](PacketInstance* pi)
    {
        selfData->pid = pi->GetIntValue("pid");

        c.log->LogDrivel("Got Self PID change to %i", selfData->pid);
    };

    std::function<void(PacketInstance*)> playerLeaving = [this](PacketInstance* pi)
    {
        i32 pid = pi->GetIntValue("pid");

        idToPlayerMap.erase(pid);
        c.players->UpdatePlayerList();

        c.log->LogDrivel("Player Left (pid=%i)", pid);
    };

    std::function<void(PacketInstance*)> playerEntering = [this](PacketInstance* pi)
    {
        shared_ptr<PlayerData> player;
        i32 pid = pi->GetIntValue("pid");

        if (pid == selfData->pid)
            player = selfData;
        else
            player = make_shared<PlayerData>();

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

        c.log->LogDrivel("Player Entering (pid=%i,name=%s)", player->pid, player->name.c_str());
    };

    std::function<void(PacketInstance*)> freqChange = [this](PacketInstance* pi)
    {
        i32 pid = pi->GetIntValue("pid");
        i32 freq = pi->GetIntValue("freq");

        idToPlayerMap[pid]->freq = freq;

        c.players->UpdatePlayerList();

        c.log->LogDrivel("Freq Change to %i (pid=%i)", freq, pid);
    };

    std::function<void(PacketInstance*)> freqShipChanged = [this](PacketInstance* pi)
    {
        i32 pid = pi->GetIntValue("pid");
        i32 freq = pi->GetIntValue("freq");
        ShipType ship = Ship_Spec;

        i32 shipNum = pi->GetIntValue("ship");

        if (shipNum >= 0 && shipNum <= 8)
            ship = (ShipType)shipNum;

        idToPlayerMap[pid]->freq = freq;
        idToPlayerMap[pid]->ship = ship;

        c.players->UpdatePlayerList();

        c.log->LogDrivel("FreqShip Change to freq=%i, ship=%i (pid=%i)", freq, shipNum, pid);
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

shared_ptr<PlayerData> Players::GetPlayerData(i32 pid)
{
    shared_ptr<PlayerData> rv = nullptr;
    auto it = data->idToPlayerMap.find(pid);

    if (it == data->idToPlayerMap.end())
        rv = nullptr;
    else
        rv = it->second;

    if (rv == nullptr)
        c.log->LogError("Players::GetPlayerData(id) failed to find player with pid %i", pid);

    return rv;
}

void Players::UpdatePlayerList()
{
    data->UpdatePlayerList();
}
