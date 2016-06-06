#include "Net.h"
#include "Chat.h"
#include <string.h>
#include <list>
#include "SDL2/SDL_net.h"

struct NetData
{
    struct QueuedReliablePacket
    {
        QueuedReliablePacket(vector<u8>* _data, u32 _ackId, i32 resendMs)
        {
            data = *_data;
            ackId = _ackId;

            timesSent = 1;
            msUntilNextSend = resendMs;
        }

        u32 ackId;
        vector<u8> data;

        i32 msUntilNextSend;
        i32 timesSent;
    };

    struct StreamData
    {
        StreamData() { reset(); };
        void reset()
        {
            data.clear();
            len = -1;
            pos = 0;
            abortFunc = nullptr;
            progressFunc = nullptr;
        };

        vector<u8> data;
        i32 len = -1;
        int pos = 0;

        std::function<void()> abortFunc;
        std::function<void(i32, i32)> progressFunc;  // gotBytes, totalBytes
    };

    const i32 MAX_BYTES_PER_PACKET = 512;
    const u8 CORE_HEADER = 0x00;
    const u8 RELIABLE_HEADER = 0x03;

    Client& c;

    i32 maxStreamLen = c.cfg->GetInt("Net", "Max File Size", 4194304);
    i32 reliableResendTime = c.cfg->GetInt("Net", "Reliable Resend Mills", 300);
    i32 reliableWarnRetries = c.cfg->GetInt("Net", "Reliable Warn Retries", 5);
    i32 reliableMaxRetries = c.cfg->GetInt("Net", "Reliable Max Retries", 10);
    i32 maxTimeWithoutData = c.cfg->GetInt("Net", "Max Time Without Data", 10);

    UDPsocket sock = nullptr;
    i32 channel = -1;
    UDPpacket* packet = nullptr;
    i32 lastData = 0;

    vector<vector<u8>> packetQueue;
    StreamData streamDataIn;
    list<QueuedReliablePacket> relPackets;
    map<u32, vector<u8>> backloggedPackets;
    u32 nextIncomingReliableId = 0;
    u32 nextOutgoingReliableId = 0;

    multimap<string, std::function<void(const PacketInstance*)>> nameToFunctionMap;
    map<u16, map<u16, string>> incomingPacketIdToLengthToNameMap;  // second one is a length map

    multimap<u8, std::function<void(u8*, i32)>> coreHandlerFuncMap;
    multimap<u8, std::function<void(u8*, i32)>> gameHandlerFuncMap;

    NetData(Client& c) : c(c) {}

    ~NetData()
    {
        ResetConnectionResources();  // will close socket and free resources
    }

    void ResetConnectionResources()
    {
        // unbind
        if (channel != -1)
        {
            SDLNet_UDP_Unbind(sock, channel);
            channel = -1;
        }

        // close
        if (sock != nullptr)
        {
            SDLNet_UDP_Close(sock);
            sock = NULL;
        }

        // and free our reusable packet
        if (packet != nullptr)
        {
            SDLNet_FreePacket(packet);
            packet = nullptr;
        }

        // reset state of reliable packets
        nextIncomingReliableId = 0;
        nextOutgoingReliableId = 0;

        relPackets.clear();
        backloggedPackets.clear();
    }

    // returns true if it was setup correctly
    bool SetupSocket(const char* hostname, u16 port)
    {
        bool rv = false;

        ResetConnectionResources();

        sock = SDLNet_UDP_Open(0);

        if (!sock)
            c.log->LogError("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
        else
        {
            IPaddress ip;

            if (SDLNet_ResolveHost(&ip, hostname, port) == -1)
                c.log->LogError("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
            else
            {
                channel = SDLNet_UDP_Bind(sock, -1, &ip);

                if (channel == -1)
                    c.log->LogError("SDLNet_UDP_Bind: %s\n", SDLNet_GetError());
                else
                {
                    // allocate our reusable packet once (freed on disconnect)
                    packet = SDLNet_AllocPacket(MAX_BYTES_PER_PACKET);

                    if (!packet)
                        c.log->LogError("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
                    else
                    {
                        lastData = SDL_GetTicks();  // so we don't disconnect right away

                        rv = true;
                    }
                }
            }
        }

        // on error, free any resources that we successfully acquired
        if (!rv)
            ResetConnectionResources();

        return rv;
    }

    void SetStatus(const char* message) { c.chat->InternalMessage(message); }

    void ResendLostReliablePackets(i32 ms)
    {
        for (list<QueuedReliablePacket>::iterator i = relPackets.begin(); i != relPackets.end();
             ++i)
        {
            i->msUntilNextSend -= ms;

            if (i->msUntilNextSend < 0)  // resend it
            {
                ++(i->timesSent);

                if (i->timesSent >= reliableMaxRetries)
                {
                    // too many reliable reties; disconnect
                    c.connection->Disconnect();

                    SetStatus("You have disconnected from the server. (too many reliable retries)");
                }
                else
                {
                    i->msUntilNextSend = reliableResendTime;
                    packetQueue.push_back(i->data);

                    if (i->timesSent >= reliableWarnRetries)
                    {
                        char buf[128];

                        snprintf(buf, sizeof(buf),
                                 "Warning: Resending reliable packet. Attempt: %i", i->timesSent);
                        c.chat->InternalMessage(buf);
                    }
                }
            }
        }
    }

    void ProcessPacket(bool core, u8 type, u8* data, int len)
    {
        pair<multimap<u8, void (*)(u8*, int)>::iterator, multimap<u8, void (*)(u8*, int)>::iterator>
            range;

        if (core)
            range = coreHandlerFuncMap.equal_range(type);
        else
            range = gameHandlerFuncMap.equal_range(type);

        if (range.first == range.second)  // no handlers
            c.log->LogError("Packet received of type %02x (%s) len = %i, but no handler exists!",
                            type, core ? "core packet" : "non-core packet", len);

        // call all of the handlers for this type
        for (; range.first != range.second; ++range.first)
            range.first->second(data, len);
    }

    void PumpPacket(u8* data, int len)
    {
        if (len > 0)
        {
            int type = -1;
            bool core = false;

            if (data[0] == 0x00)  // core packet
            {
                if (len > 1)
                {
                    core = true;
                    type = data[1];
                }
                else
                    LOG_ERROR("core packet with length < 2");
            }
            else
                type = data[0];

            if (type != -1)
                ProcessPacket(core, (u8)type, data, len);
        }
        else
            c.log->LogError("pumpPacket's packetlen <= 0");
    }

    void PollSocket(i32 ms)
    {
        i32 now = SDL_GetTicks();

        while (SDLNet_UDP_Recv(sock, packet))
        {
            // dumpPacket("RECV", (u8*)packet->data, packet->len);

            lastData = now;
            PumpPacket(packet->data, packet->len);
        }

        if (now - lastData > maxTimeWithoutData)  // we should disconnect, no data
        {
            PacketInstance pi;
            c.packets->SendPacket(&pi, "disconnect");

            SetStatus("You have disconnected from the server (no data coming in).");
            c.connection->Disconnect();
        }
    }

    void AddPacketHandler(const char* name, std::function<void(const PacketInstance*)> func)
    {
        pair<string, std::function<void(const PacketInstance*)>> toInsert(name, func);

        nameToFunctionMap.insert(toInsert);

        PacketTemplate* pt = GetTemplate(name, false);

        if (pt)
        {
            u16 type = pt->isCore ? pt->type : (pt->type << 8);

            if (listeningForTypes.find(type) == listeningForTypes.end())
            {  // insert and register
                c.net->RegisterPacketHandler(pt->isCore, pt->type, templatePacketRecevied);

                listeningForTypes.insert(type);
            }
        }
    }

    void AddRawPacketHandler(bool core, u8 type, std::function<void(u8*, i32)> func)
    {
        if (core)
            coreHandlerFuncMap.insert(pair<u8, void (*)(u8*, int)>(type, func));
        else
            gameHandlerFuncMap.insert(pair<u8, void (*)(u8*, int)>(type, func));
    }
};

Net::Net(Client& c) : Module(c), data(make_shared<NetData>(c))
{
    // todo: figure out the ordering on this
    /*registerPacketHandler(true, 0x03, handleReliablePacket);
    registerPacketHandler(true, 0x0E, handleClusterPacket);
    registerPacketHandler(true, 0x0A, handleStream);

    // parsed
    c.packets->regPacketFunc("cancel stream response", handleCancelStreamResponse);
    mm->net.regPacketFunc("sync pong", handleSyncPong);
    mm->net.regPacketFunc("sync request", handleSyncRequest);
    mm->net.regPacketFunc("keep alive", handleKeepAlive);
    mm->net.regPacketFunc("reliable response", handleReliableReponse);
    mm->net.regPacketFunc("encryption response", handleEncryptionResponse);
    mm->net.regPacketFunc("password response", handlePasswordResponse);
    mm->net.regPacketFunc("disconnect", handleDisconnect);
    */
}

Net::~Net()
{
}

bool Net::NewConnection(const char* hostname, u16 port)
{
    bool rv = data->SetupSocket(hostname, port);

    if (rv)
    {
    }

    return rv;
}

void Net::DisconnectSocket()
{
    // if we're connected, send a disconnect packet
    if (data->packet != nullptr)
    {
        c.connection->Disconnect();
    }

    data->ResetConnectionResources();
}

void Net::ReceivePackets(i32 ms)
{
    if (data->sock != nullptr)
    {
        data->PollSocket(ms);
    }
}

void Net::SendPackets(i32 ms)
{
    data->ResendLostReliablePackets(ms);
}

void Net::AddPacketHandler(const char* name, std::function<void(const PacketInstance*)> func)
{
    data->AddPacketHandler(name, func);
}

void Net::SendPacket(PacketInstance* packet, const char* templateName)
{
}

void Net::SendReliablePacket(PacketInstance* packet, const char* templateName)
{
}
