// Stanley Bak
// Additional function for handling core features (used within Net module)
// June 2016

#pragma once

#include "Client.h"
#include "Connection.h"
#include <vector>
#include <list>
using namespace std;

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
        u32 id = nextOutgoingReliableId;

        // Little Endian PutU32
        for (int x = 2; x < 6; ++x)
        {
            (*packet)[x] = id % 256;
            id /= 256;
        }

        relPackets.push_back(
            QueuedReliablePacket(packet, nextOutgoingReliableId, reliableResendTime));

        ++nextOutgoingReliableId;
    }
};
