#pragma once

#include "Module.h"

struct MapData;

unique_ptr<u8[]> ZlibDecompress(Client& c, const u8* src, int len, i32* decompressedLen);

class Map : public Module
{
   public:
    Map(Client& c);

    void GotMapInfo(const char* filename, u32 checksum, u32 compressedSize);

    void SetMapPath(const char* filename);  // use nullptr to clear map

   private:
    shared_ptr<MapData> data;
};
