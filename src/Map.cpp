#include "Map.h"
#include "Connection.h"
#include "Graphics.h"
#include "Packets.h"
#include "Net.h"
#include "zlib.h"
#include <fstream>
using namespace std;

unique_ptr<u8[]> ZlibDecompress(Client& c, const u8* src, int len, i32* decompressedLen)
{
    unique_ptr<u8[]> rv;
    int decompressedArraySize = len;
    u64 decompressedLenU64 = 0;

    while (true)
    {
        decompressedArraySize *= 2;
        rv = unique_ptr<u8[]>(new u8[decompressedArraySize]);
        decompressedLenU64 = decompressedArraySize;

        u8* ptr = rv.get();
        int result = uncompress(ptr, &decompressedLenU64, src, len);

        if (result == Z_BUF_ERROR)
            continue;  // retry with larger buffer
        else if (result != Z_OK)
            c.log->FatalError("Error while zlib decompressing: %d", result);
        else
            break;
    }

    *decompressedLen = (i32)decompressedLenU64;

    c.log->LogDrivel(
        "zlib uncompress succeeded. compressed=%d bytes, uncompressed=%d bytes (ratio = %.1f%%)",
        len, *decompressedLen, 100.0 * len / *decompressedLen);

    return rv;
}

struct TileStruct
{
    unsigned x : 12;  // range 0-1023, upper two bits should be 0
    unsigned y : 12;
    unsigned tile : 8;
};

struct MapData
{
    const int MAP_TILES_WIDTH = 1024;
    const int MAP_TILES_HEIGHT = 1024;
    const int PIXELS_PER_TILE = 16;

    MapData(Client& c) : c(c) {}

    Client& c;
    bool hasTileset = false;
    unique_ptr<u8[]> theMap;

    shared_ptr<Image> defaultTileset = c.graphics->LoadImage("default_tileset", 19, 10);
    shared_ptr<Image> curTileset;

    void GotMapInfo(const char* filename, u32 checksum, u32 compressedSize)
    {
        // check if it exists
        string zoneName;
        string arenaName;

        string dir = c.connection->GetZoneDir();

        string mapFilename = dir + filename;

        if (!ValidateFile(mapFilename.c_str(), checksum))
        {
            c.log->LogDrivel("Requesting download of map file '%s'", mapFilename.c_str());

            // mapCompressedSize = compressedSize;
            c.net->ExpectStreamTransfer(dlAbortFunc, dlProgressFunc);

            // download new version of map
            PacketInstance pi("map request");
            c.net->SendReliablePacket(&pi);
        }
        else
        {
            c.log->LogDrivel("Existing map file '%s' had correct crc32", mapFilename.c_str());

            c.map->SetMapPath(mapFilename.c_str());
        }
    }

    std::function<void()> dlAbortFunc = [this]()
    {
        c.log->LogError("Map Download was cancelled. Disconnecting.");
        c.connection->Disconnect();
    };

    std::function<void(i32, i32)> dlProgressFunc = [this](i32 got, i32 total)
    {
        c.log->LogDrivel("Download Progress: %d/%d (%0.1f%%)", got, total, (100.0 * got / total));
    };

    bool ValidateFile(const char* path, u32 crc32)
    {
        u32 mycrc = 0;
        bool rv = GetCrc32(path, &mycrc);

        if (!rv || (crc32 != mycrc))
            rv = false;

        return rv;
    }

    bool GetCrc32(const char* filename, u32* storeCrc32)
    {
        FILE* f;
        char buf[8192];
        bool rv = false;

        u32 crc = crc32(0, Z_NULL, 0);

        f = fopen(filename, "rb");

        if (f)
        {
            rv = true;

            while (!feof(f))
            {
                int b = (int)fread(buf, 1, sizeof(buf), f);
                crc = crc32(crc, (const Bytef*)buf, b);
            }

            fclose(f);
        }

        *storeCrc32 = crc;

        return rv;
    }

    // returns the start of the tileset data or 0 if there is no tileset in the file
    int GetTileDataStart(ifstream& fin)
    {
        int rv = 0;

        u8 data[16];

        fin.read((char*)data, 2);

        if (!fin)
            c.log->FatalError("Error reading map file header.");

        // if the bitmap header exists
        if (data[0] == 'B' && data[1] == 'M')
        {
            fin.read((char*)data, 4);

            if (!fin)
                c.log->FatalError("Error reading map file tile data start.");

            rv = GetU32(data);
        }

        return rv;
    }

    void ReadTile(ifstream& fin, TileStruct* t)
    {
        u8 data[4];
        fin.read((char*)data, sizeof(data));

        // little endian black magic
        t->x = data[0] | ((data[1] & 0x0F) << 8);
        t->y = ((data[1] & 0xF0) >> 4) | (data[2] << 4);
        t->tile = data[3];
    }

    void SetMapPath(const char* path)
    {
        if (path == nullptr)
        {
            // clear associated map data
            hasTileset = false;
            theMap = nullptr;
            curTileset = nullptr;
        }
        else
        {
            // load it
            ifstream fin(path, ios::binary);

            if (fin.good())
            {
                // find the start of the tile data
                int startOfTileData = GetTileDataStart(fin);
                bool hasTileset = startOfTileData != 0;

                fin.seekg(startOfTileData, ios_base::beg);

                int mapBytes = MAP_TILES_WIDTH * MAP_TILES_HEIGHT;
                theMap = unique_ptr<u8[]>(new u8[mapBytes]);

                for (int x = 0; x < MAP_TILES_WIDTH; ++x)
                    for (int y = 0; y < MAP_TILES_HEIGHT; ++y)
                        theMap[y * MAP_TILES_WIDTH + x] = 0;

                while (!fin.eof())
                {
                    TileStruct t;

                    ReadTile(fin, &t);

                    if (!fin.eof())
                    {
                        if (t.x < MAP_TILES_WIDTH && t.y < MAP_TILES_HEIGHT)
                            theMap[t.y * MAP_TILES_WIDTH + t.x] = t.tile;
                        else
                            c.log->FatalError("Out of bounds tile coordinate in lvl file.");
                    }
                }

                fin.close();

                if (hasTileset)
                {
                    // prepend "../" to get out of the resources folder
                    string tilesetPath = string("../") + path;
                    curTileset = c.graphics->LoadImage(tilesetPath.c_str());
                }
                else
                    curTileset = defaultTileset;
            }
            else
                c.log->FatalError("Error loading map from '%s'", path);

            c.log->LogDrivel("Map successfully loaded from '%s'", path);
        }

        // ok map is now loaded, we should set up the drawing of it, maybe use players to store
        // player positions which will center the screen on the current player
    }

    std::function<void(const PacketInstance*)> handleIncomingCompressedMap =
        [this](const PacketInstance* pi)
    {
        const string* receivedFilename = pi->GetStringValue("filename");
        const vector<u8>* compressedData = pi->GetRawValue("data");

        i32 decompressedLen = 0;
        unique_ptr<u8[]> data =
            ZlibDecompress(c, &((*compressedData)[0]), compressedData->size(), &decompressedLen);

        string dir = c.connection->GetZoneDir();
        string savePath = dir + SanitizeString(receivedFilename->c_str());

        FILE* f = fopen(savePath.c_str(), "wb");

        if (!f)
            c.log->FatalError("problem writing to file (write protected?): '%s'", savePath.c_str());

        if (fwrite(data.get(), decompressedLen, 1, f) != 1)
            c.log->FatalError("partial write to map file: '%s'", savePath.c_str());

        fclose(f);

        c.log->LogDrivel("Save downloaded map to '%s'", savePath.c_str());

        c.map->SetMapPath(savePath.c_str());
    };

    void DrawMap()
    {
        shared_ptr<Player> p = c.players->GetSelfPlayer();
        i32 playerX = p->GetXPixel();
        i32 playerY = p->GetYPixel();
        u32 w = 0, h = 0;

        c.graphics->GetScreenSize(&w, &h);

        i32 topPixel = playerY - h / 2;
        i32 topTile = topPixel / PIXELS_PER_TILE - 1;
        i32 bottomTile = topTile + h / PIXELS_PER_TILE + 2;

        i32 leftPixel = playerX - w / 2;
        i32 leftTile = leftPixel / PIXELS_PER_TILE - 1;
        i32 rightTile = leftTile + w / PIXELS_PER_TILE + 2;

        i32 drawOffsetX = w / 2 - playerX;
        i32 drawOffsetY = h / 2 - playerY;

        for (i32 y = topTile; y < bottomTile; ++y)
        {
            if (y < 0 || y >= MAP_TILES_HEIGHT)
                continue;

            for (i32 x = leftTile; x < rightTile; ++x)
            {
                if (x < 0 || x >= MAP_TILES_WIDTH)
                    continue;

                u8 tile = theMap[y * MAP_TILES_WIDTH + x];

                if (tile > 0 && tile <= 190)  // over 162 is special tile?
                {
                    i32 xpos = x * 16 + drawOffsetX;
                    i32 ypos = y * 16 + drawOffsetY;

                    c.graphics->DrawImageFrame(curTileset, tile - 1, xpos, ypos);
                }
            }
        }
    }
};

Map::Map(Client& c) : Module(c), data(make_shared<MapData>(c))
{
    c.net->AddPacketHandler("incoming compressed map", data->handleIncomingCompressedMap);
}

void Map::GotMapInfo(const char* filename, u32 checksum, u32 compressedSize)
{
    data->GotMapInfo(filename, checksum, compressedSize);
}

void Map::SetMapPath(const char* path)
{
    data->SetMapPath(path);
}

void Map::DrawMap()
{
    if (data->theMap)
        data->DrawMap();
}
