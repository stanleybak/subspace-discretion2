/*
 * packets.h
 *
 *  Created on: Jun 4, 2016
 *      Author: stan
 */

#pragma once

#include "Module.h"
#include <vector>

struct PacketsData;

struct PacketInstance
{
    PacketInstance(const char* templateName) : templateName(templateName) {}
    void setValue(const char* type, i32 value) { intValues[type] = value; }
    void setValue(const char* type, const char* value) { cStrValues[type] = value; }
    void setValue(const char* type, const vector<u8>* value) { rawValues[type] = *value; }

    string templateName;
    map<string, i32> intValues;
    map<string, string> cStrValues;
    map<string, vector<u8> > rawValues;
};

typedef pair<bool, u8> PacketType;  // isCore, id

class Packets : public Module
{
   public:
    Packets(Client& c);
    ~Packets();

    // never fails
    PacketType GetPacketType(const char* templateName, bool isOutgoing);

    // returns the template name or nullptr, if not found
    void PopulatePacketInstance(PacketInstance* store, u8* data, int len);

    // returns false and logs error if malformed
    bool CheckPacket(PacketInstance* pi, bool reliable);

    void PacketTemplateToRaw(PacketInstance* packet, bool reliable, vector<u8>* rawData);

   private:
    shared_ptr<PacketsData> data;
};
