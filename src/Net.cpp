#include "Net.h"
#include "Chat.h"
#include "NetCoreHandlers.h"
#include "Connection.h"

#include "SDL2/SDL_net.h"
#include <list>

struct NetData
{
    const u32 MAX_BYTES_PER_PACKET = 512;
    const u32 MAX_CLUSTER_SIZE = 256;

    Client& c;
    NetCoreHanders coreHandlers;

    i32 maxTimeWithoutData = c.cfg->GetInt("Net", "Max Time Without Data", 10);

    UDPsocket sock = nullptr;
    i32 channel = -1;
    UDPpacket* packet = nullptr;
    i32 lastData = 0;

    vector<vector<u8>> packetQueue;

    multimap<string, std::function<void(const PacketInstance*)>> nameToFunctionMap;
    multimap<PacketType, std::function<void(u8*, i32)>> rawPackedHandlers;

    NetData(Client& c) : c(c), coreHandlers(c) {}

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
        coreHandlers.Reset();
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

    void ProcessRawTypedPacket(PacketType type, u8* data, int len)
    {
        auto range = rawPackedHandlers.equal_range(type);

        if (range.first == range.second)  // no handlers
            c.log->LogError("Packet received of type %04x (%s) len = %i, but no handler exists!",
                            type.second, type.first ? "core packet" : "non-core packet", len);

        // call all of the handlers for this type
        for (; range.first != range.second; ++range.first)
            range.first->second(data, len);
    }

    void PumpPacket(u8* data, int len)
    {
        if (len < 1 || (data[0] == CORE_HEADER && len < 2))
            c.log->LogError("pumpPacket's packet len invalid");
        else
        {
            PacketType type;

            if (data[0] == CORE_HEADER)  // core packet
            {
                type.first = true;
                type.second = data[1];
            }
            else
            {
                type.first = false;
                type.second = data[0];
            }

            ProcessRawTypedPacket(type, data, len);
        }
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
            c.chat->InternalMessage("You have disconnected from the server (no data coming in).");
            c.connection->Disconnect();
        }
    }

    void AddPacketHandler(const char* name, std::function<void(const PacketInstance*)> func)
    {
        pair<string, std::function<void(const PacketInstance*)>> toInsert(name, func);

        if (nameToFunctionMap.find(name) == nameToFunctionMap.end())
        {
            PacketType type = c.packets->GetPacketType(name, false);
            rawPackedHandlers.insert(make_pair(type, templatePacketRecevied));
        }

        nameToFunctionMap.insert(toInsert);
    }

    void AddRawPacketHandler(PacketType type, std::function<void(u8*, i32)> func)
    {
        rawPackedHandlers.insert(make_pair(type, func));
    }

    void SendPacket(PacketInstance* packet, bool reliable)
    {
        if (c.packets->CheckPacket(packet, reliable))
        {
            vector<u8> rawData;

            c.packets->PacketTemplateToRaw(packet, reliable, &rawData);

            // reliable messages need an id assigned and must be put in a resend queue
            if (reliable)
                coreHandlers.FinalizeReliablePacket(&rawData);

            // put it in the vector of stuff to send
            packetQueue.push_back(rawData);
        }
    }

    void SendBinaryPacket(UDPpacket* p)
    {
        // statsSentRecently += p->len;
        // statsSentTotal += p->len;

        if (!SDLNet_UDP_Send(sock, channel, p))
            c.log->LogError("SDLNet_UDP_Send: %s\n", SDLNet_GetError());
    }

    void FlushRawOutgoingPackets()
    {
        // cluster as many packets as we can and send them
        unsigned int numPackets = 0;
        unsigned int curByte = 2;
        packet->len = 2;
        packet->data[0] = CORE_HEADER;
        packet->data[1] = CLUSTER_HEADER;

        for (unsigned int x = 0; x < packetQueue.size(); ++x)
        {
            // for a packet to be clusterable, size must be < 256
            if (packetQueue[x].size() < MAX_CLUSTER_SIZE)  // cluster it
            {
                if (curByte + packetQueue[x].size() + 1 >=
                    MAX_BYTES_PER_PACKET)  // if we can't append the next cluster
                {                          // send it and setup for the next clustered packet
                    SendBinaryPacket(packet);

                    curByte = 2;
                    packet->len = 2;
                    packet->data[0] = CORE_HEADER;
                    packet->data[1] = CLUSTER_HEADER;
                    numPackets = 0;
                }

                packet->len += (int)(1 + packetQueue[x].size());
                packet->data[curByte++] = (u8)packetQueue[x].size();

                for (unsigned int c = 0; c < packetQueue[x].size(); ++c)
                    packet->data[curByte++] = packetQueue[x][c];

                packetQueue[x].clear();
                ++numPackets;
            }
        }

        // flush the buffer (if we have any buffered packets)
        if (numPackets > 1)
        {  // send the clustered version
            SendBinaryPacket(packet);
        }
        else if (numPackets == 1)
        {  // send the non-clustered version
            for (int c = 0; c < packet->len - 3; ++c)
                packet->data[c] = packet->data[c + 3];

            packet->len -= 3;
            SendBinaryPacket(packet);
        }

        // finally, send all the packets we couldn't cluster (because size was too large, perhaps)
        for (unsigned int x = 0; x < packetQueue.size(); ++x)
        {
            if (packetQueue[x].size() > MAX_BYTES_PER_PACKET)
            {
                bool isCore = false;
                u8 id = packetQueue[x][0];

                if (packetQueue[x][0] == CORE_HEADER)
                {
                    isCore = true;
                    id = packetQueue[x][1];
                }

                c.log->LogError(
                    "queued packet's size was > MAX_BYTES_PER_PACKET, dropping packet."
                    " Id was 0x%02x (%s).",
                    id, isCore ? "core packet" : "non-core packet");
            }
            else if (packetQueue[x].size() > 0)  // we haven't sent it already
            {
                packet->len = (int)packetQueue[x].size();

                for (unsigned int c = 0; c < packetQueue[x].size(); ++c)
                    packet->data[x] = packet->data[c];

                SendBinaryPacket(packet);
            }
        }

        packetQueue.clear();
    }

    // generic raw handler for all template functions which have handler function
    std::function<void(u8*, i32)> templatePacketRecevied = [this](u8* data, i32 len)
    {
        PacketInstance store("temp");
        c.packets->PopulatePacketInstance(&store, data, len);

        if (store.templateName != "temp")
        {
            // lookup the template handler
            pair<multimap<string, std::function<void(const PacketInstance*)>>::iterator,
                 multimap<string, std::function<void(const PacketInstance*)>>::iterator> range =
                nameToFunctionMap.equal_range(store.templateName);

            for (; range.first != range.second; ++range.first)
                range.first->second(&store);
        }
    };
};

Net::Net(Client& c) : Module(c), data(make_shared<NetData>(c))
{
    printf("Net.cpp todo: Add these in the right places!\n");
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
        if (c.connection->isDisconnected())
            c.log->LogError(
                "Net::DisconnectSocket() called, but connection is still active (will lead to "
                "dirty disconnect). Use Connection::Disconnect() instead.");

        data->FlushRawOutgoingPackets();
    }

    data->ResetConnectionResources();
}

void Net::ReceivePackets(i32 ms)
{
    if (data->sock != nullptr)
        data->PollSocket(ms);
}

void Net::SendPackets(i32 ms)
{
    data->coreHandlers.ResendReliablePackets(ms, &data->packetQueue);

    data->FlushRawOutgoingPackets();
}

void Net::AddPacketHandler(const char* name, std::function<void(const PacketInstance*)> func)
{
    data->AddPacketHandler(name, func);
}

void Net::SendPacket(PacketInstance* packet)
{
    data->SendPacket(packet, false);
}

void Net::SendReliablePacket(PacketInstance* packet)
{
    data->SendPacket(packet, true);
}
