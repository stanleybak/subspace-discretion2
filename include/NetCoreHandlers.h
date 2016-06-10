// Stanley Bak
// Additional function for handling core features (used within Net module)
// June 2016

#pragma once

#include "Client.h"
#include "Connection.h"
#include <SDL/SDL.h>
#include <vector>
#include <list>
using namespace std;

const int STREAM_LEN_UNINITIALIZED = -1;
const int STREAM_LEN_EXPECTING = -2;

struct NetCoreHanders
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
            len = STREAM_LEN_UNINITIALIZED;
            pos = 0;
            abortFunc = nullptr;
            progressFunc = nullptr;
        };

        vector<u8> data;
        i32 len = STREAM_LEN_UNINITIALIZED;
        int pos = 0;

        std::function<void()> abortFunc;
        std::function<void(i32, i32)> progressFunc;  // gotBytes, totalBytes
    };

    Client& c;
    i32 maxStreamLen = c.cfg->GetInt("Net", "Max File Size", 4194304);
    i32 reliableResendTime = c.cfg->GetInt("Net", "Reliable Resend Mills", 300);
    i32 reliableWarnRetries = c.cfg->GetInt("Net", "Reliable Warn Retries", 5);
    i32 reliableMaxRetries = c.cfg->GetInt("Net", "Reliable Max Retries", 10);

    StreamData streamDataIn;
    list<QueuedReliablePacket> relPackets;
    map<u32, vector<u8>> backloggedPackets;
    u32 nextIncomingReliableId = 0;
    u32 nextOutgoingReliableId = 0;

    i32 serverTimeOffset = 0;  // the amount of centiseconds the server is ahead of us

    NetCoreHanders(Client& c) : c(c){};

    void Reset()
    {
        nextIncomingReliableId = 0;
        nextOutgoingReliableId = 0;

        relPackets.clear();
        backloggedPackets.clear();
    }

    void ResendReliablePackets(i32 ms, vector<vector<u8>>* packetQueue)
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

                    c.chat->InternalMessage(
                        "You have disconnected from the server. (too many reliable retries)");
                }
                else
                {
                    i->msUntilNextSend = reliableResendTime;
                    packetQueue->push_back(i->data);

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

    void FinalizeReliablePacket(vector<u8>* packet)
    {
        if (packet->size() < 6)
            c.log->FatalError("Attempted to send reliable packet with length < 6.");

        // write the id to the packet data in indices 2-5
        u32 id = nextOutgoingReliableId++;

        // Little Endian PutU32
        u8 data[4];
        PutU32(data, id);

        const i32 RELIABLE_ID_OFFSET = 2;

        for (int x = 0; x < 4; ++x)
            (*packet)[RELIABLE_ID_OFFSET + x] = data[x];

        relPackets.push_back(QueuedReliablePacket(packet, id, reliableResendTime));
    }

    std::function<void(u8*, int)> handleReliablePacket = [this](u8* data, int len)
    {
        if (len < 6)
            c.log->LogError("Got Reliable packet of length < 6");
        else
        {
            u32 ackId;
            ackId = GetU32(data + 2);

            // send ack
            if (ackId <= nextIncomingReliableId)  // this ensures ordering
            {
                PacketInstance pi(c, "reliable response");
                pi.SetValue("id", ackId);
                c.net->SendPacket(&pi);
            }
            else
            {
                // backlog it

                if (backloggedPackets.find(ackId) == backloggedPackets.end())
                {
                    vector<u8> vecData(len);

                    for (int i = 0; i < len; ++i)
                        vecData[i] = data[i];

                    backloggedPackets[ackId] = vecData;
                }
            }

            // and handle the packet (if necessary)
            if (ackId == nextIncomingReliableId)
            {
                c.net->PumpPacket(data + 6, len - 6);

                // and increment the next reliable ack id
                ++nextIncomingReliableId;

                // and loop over backloggedPackets
                for (map<u32, vector<u8>>::iterator i =
                         backloggedPackets.find(nextIncomingReliableId);
                     i != backloggedPackets.end();
                     i = backloggedPackets.find(++nextIncomingReliableId))
                {
                    // send ack
                    PacketInstance pi(c, "reliable response");
                    pi.SetValue("id", nextIncomingReliableId);
                    c.net->SendPacket(&pi);

                    // pump it
                    u8* firstElement = &i->second[0];
                    c.net->PumpPacket(firstElement + 6, i->second.size() - 6);

                    // remove it
                    backloggedPackets.erase(i);
                }
            }
        }
    };

    std::function<void(u8*, int)> handleClusterPacket = [this](u8* data, int len)
    {
        int curOffset = 2;

        while (curOffset < len)
        {
            int segLength = data[curOffset++];

            if (segLength > len - curOffset)
                c.log->LogError("Cluster packet received with illegal length");
            else
                c.net->PumpPacket(data + curOffset, segLength);

            curOffset += segLength;
        }
    };

    std::function<void(u8*, int)> handleStream = [this](u8* data, int packet_len)
    {
        if (packet_len < 6)
            c.log->LogError("Got Stream packet of length < 6");
        else
        {
            u32 len = GetU32(data + 2);

            if (len == 0)
                c.log->LogError("stream length = 0; not allowed");
            else if (len >= (u32)maxStreamLen)
            {
                c.log->LogError("len in stream chunk(%i) >= stream limit length(%i)", len,
                                maxStreamLen);
            }
            else
            {
                // this is the first packet, assign the length
                if (streamDataIn.len == STREAM_LEN_EXPECTING)
                    streamDataIn.len = len;

                if ((int)len != streamDataIn.len)
                {
                    c.log->LogError(
                        "length of stream transfer is not the reported length in one of the "
                        "chunks; ignoring chunk");
                }
                else  // append it to current pos
                {
                    for (u32 x = 6; x < (u32)packet_len; ++x)
                        streamDataIn.data.push_back(data[x]);

                    if ((int)streamDataIn.data.size() > streamDataIn.len)
                    {
                        c.log->LogError(
                            "server sent more data than what the stream size was; truncating data");
                        streamDataIn.data.resize(streamDataIn.len);
                    }

                    if (streamDataIn.progressFunc)
                        streamDataIn.progressFunc((int)streamDataIn.data.size(), streamDataIn.len);

                    if ((int)streamDataIn.data.size() == streamDataIn.len)
                    {
                        c.net->PumpPacket(&(streamDataIn.data[0]), streamDataIn.len);

                        streamDataIn.reset();
                    }
                }
            }
        }
    };

    std::function<void(const PacketInstance*)> handleCancelStreamResponse =
        [this](const PacketInstance* pi)
    {
        if (streamDataIn.len == STREAM_LEN_UNINITIALIZED)
            c.log->LogError("Got cancel stream ack when stream is not initialized");
        else if (streamDataIn.abortFunc)
            streamDataIn.abortFunc();

        streamDataIn.reset();
    };

    std::function<void(const PacketInstance*)> handleReliableReponse =
        [this](const PacketInstance* pi)
    {
        u32 id = pi->GetValue("id");

        // remove it from the relPackets queue
        for (list<QueuedReliablePacket>::iterator i = relPackets.begin(); i != relPackets.end();
             ++i)
        {
            if (i->ackId == id)
            {
                relPackets.erase(i);
                break;
            }
        }
    };

    std::function<void(const PacketInstance*)> handleKeepAlive = [this](const PacketInstance* pi)
    {
        // the server sends these so we don't think we've disconnected
    };

    std::function<void(const PacketInstance*)> handleSyncRequest = [this](const PacketInstance* pi)
    {
        PacketInstance p(c, "sync ping");
        p.SetValue("timestamp", SDL_GetTicks() / 10);
        c.net->SendPacket(&p);
    };

    std::function<void(const PacketInstance*)> handleSyncPong = [this](const PacketInstance* pi)
    {
        int myTime = SDL_GetTicks() / 10;
        int serverTime = pi->GetValue("server timestamp");
        int sentTime = pi->GetValue("original timestamp");
        int roundTripCentiseconds = (myTime - sentTime);

        // printf(":coreHandlers, got sync pong, time = %i, time/10=%i\n", util->getMilliseconds(),
        // myTime);

        serverTimeOffset = (serverTime + roundTripCentiseconds / 2) - myTime;
    };
};
