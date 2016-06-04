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
    PacketInstance() {}
    void setValue(const char* type, i32 value) { intValues[type] = value; }
    void setValue(const char* type, const char* value) { cStrValues[type] = value; }
    void setValue(const char* type, const vector<u8>* value) { rawValues[type] = *value; }

    int getValue(const char* type) const;
    void getValue(const char* type, char* store, int len) const;
    const u8* getValue(const char* type, int* rawLen) const;

    map<string, i32> intValues;
    map<string, string> cStrValues;
    map<string, vector<u8> > rawValues;
};

class Packets : public Module
{
   public:
    Packets(Client& c);
    ~Packets();

   private:
    shared_ptr<PacketsData> data;
};
