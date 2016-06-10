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

void PutU32(u8* loc, u32 data);
void PutU16(u8* loc, u16 data);
u32 GetU32(u8* loc);
u16 GetU16(u8* loc);

typedef pair<bool, u8> PacketType;  // isCore, id

struct PacketInstance
{
    PacketInstance(Client& c, const char* templateName) : c(c), templateName(templateName) {}
    void SetValue(const char* type, i32 value) { intValues[type] = value; }
    void SetValue(const char* type, const char* value) { cStrValues[type] = value; }
    void SetValue(const char* type, const vector<u8>* value) { rawValues[type] = *value; }

    int GetValue(const char* type) const;
    void GetValue(const char* type, char* store, int len) const;
    const u8* GetValue(const char* type, int* rawLen) const;

    Client& c;
    string templateName;
    map<string, i32> intValues;
    map<string, string> cStrValues;
    map<string, vector<u8> > rawValues;
};

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
