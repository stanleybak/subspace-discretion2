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

    i32 MAX_BYTES_PER_PACKET = 512;

    Client& c;

    i32 maxStreamLen = c.cfg->GetInt("Net", "Max File Size", 4194304);
    i32 reliableResendTime = c.cfg->GetInt("Net", "Reliable Resend Mills", 300);
    i32 reliableWarnRetries = c.cfg->GetInt("Net", "Reliable Warn Retries", 5);
    i32 reliableMaxRetries = c.cfg->GetInt("Net", "Reliable Max Retries", 10);

    UDPsocket sock = nullptr;
    i32 channel = -1;
    UDPpacket* packet = nullptr;

    vector<vector<u8> > packetQueue;
    StreamData streamDataIn;
    list<QueuedReliablePacket> relPackets;
    map<u32, vector<u8> > backloggedPackets;
    u32 nextIncomingReliableId = 0;
    u32 nextOutgoingReliableId = 0;

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
                        rv = true;
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
                    c.net->Disconnect();

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
    };
};

Net::Net(Client& c) : Module(c), data(make_shared<NetData>(c))
{
}

Net::~Net()
{
}

void Net::AddPacketHandler(const char* name, std::function<void(const PacketInstance*)> func)
{
}

void Net::SendPacket(PacketInstance* packet, const char* templateName)
{
}

void Net::SendReliablePacket(PacketInstance* packet, const char* templateName)
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

void Net::Disconnect()
{
    // if we're connected, send a disconnect packet
    if (data->packet != nullptr)
    {
        PacketInstance pi;
        SendPacket(&pi, "disconnect");
    }

    data->ResetConnectionResources();
}

void Net::SendAndReceive(i32 iterationMs)
{
    if (data->sock != nullptr)
    {
        // first receive

        // than send
        data->ResendLostReliablePackets(iterationMs);

        // flush packetQueue
    }
}
