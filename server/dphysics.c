// Discretion 2 physics module

#include <assert.h>
#include <string.h>

#include "asss.h"
#include "dphysics_packets.h"
#include "packets/types.h"

local Imodman *mm = 0;
local Inet *net = 0;
local Iarenaman *aman = 0;
local Imainloop *ml = 0;
local Iplayerdata *pd = 0;
local Ilogman *log = 0;

local int adkey = 0;

local pthread_mutex_t dphysics_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK() pthread_mutex_lock(&dphysics_mutex)
#define UNLOCK() pthread_mutex_unlock(&dphysics_mutex)

typedef struct FrameData
{
    LinkedList pidStateList;
    LinkedList playerStateList;
    LinkedList weaponStateList;
} FrameData;

typedef struct ArenaData
{
    // hmm
    u32 frameOffset;
} ArenaData;

local void initArenaData(Arena *a)
{
    LOCK();
    ArenaData *data = P_ARENA_DATA(a, adkey);

    data->frameOffset = 0;
    UNLOCK();
}

local void deinitArenaData(Arena *a)
{
    LOCK();
    // ArenaData *data = P_ARENA_DATA(a, adkey);

    // LLFree(&data->frames);
    UNLOCK();
}

local void initFrameData(FrameData *fd)
{
    LLInit(&fd->pidStateList);
    LLInit(&fd->playerStateList);
    LLInit(&fd->weaponStateList);
}

local void deinitFrameData(FrameData *fd)
{
    LLEmpty(&fd->pidStateList);
    LLEmpty(&fd->playerStateList);
    LLEmpty(&fd->weaponStateList);
}

local void ppk(Player *p, byte *pkt, int len)
{
    LOCK();
    // struct C2SPosition *pos = (struct C2SPosition *)pkt;
    UNLOCK();
}

// allocate memory for frame packet, return it, and set size parameter
local byte *makeFramePacket(FrameData *fd, u32 *size)
{
    FrameHeader header;

    // Note: LLCount is not the most efficient function...
    header.numPidStates = LLCount(&fd->pidStateList);
    header.numPlayerStates = LLCount(&fd->playerStateList);
    header.numWeaponStates = LLCount(&fd->weaponStateList);

    *size = sizeof(FrameHeader) + header.numPidStates * sizeof(PidState) +
            header.numPlayerStates * sizeof(PlayerState) +
            header.numWeaponStates * sizeof(WeaponState);

    byte *rv = amalloc(*size);
    byte *curOffset = rv;

    memcpy(curOffset, &header, sizeof(FrameHeader));
    curOffset += sizeof(FrameHeader);

    Link *link = 0;

    // copy pid state
    PidState *curPidState = 0;

    FOR_EACH(fd->pidStateList, curPidState, link)
    {
        memcpy(curOffset, curPidState, sizeof(PidState));
        curOffset += sizeof(PidState);
    }

    // copy player state
    PlayerState *curPlayerState = 0;

    FOR_EACH(fd->playerStateList, curPlayerState, link)
    {
        memcpy(curOffset, curPlayerState, sizeof(PlayerState));
        curOffset += sizeof(PlayerState);
    }

    // copy weapon state
    WeaponState *curWeaponState = 0;

    FOR_EACH(fd->weaponStateList, curWeaponState, link)
    {
        memcpy(curOffset, curWeaponState, sizeof(WeaponState));
        curOffset += sizeof(WeaponState);
    }

    assert(curOffset == rv + *size);

    return rv;
}

local void sendFrameDataToAllPlayers(FrameData *fd, Arena *arena)
{
    Player *p = NULL;
    Link *link = NULL;
    u32 size = 0;
    byte *packet = makeFramePacket(fd, &size);

    net->SendToArena(arena, NULL, packet, size, NET_UNRELIABLE);

    afree(packet);
}

local int FrameTimer(void *dummy)
{
    Arena *arena = NULL;
    Link *link = NULL;
    LOCK();

    FOR_EACH_ARENA(arena)
    {
        FrameData fd;
        initFrameData(&fd);

        populateFrameData(&fd);
        sendFrameDataToAllPlayers(&fd, arena);

        deinitFrameData(&fd);
    }

    UNLOCK();

    return TRUE;  // keep running
}

local void paction(Player *p, int action, Arena *arena)
{
    LOCK();

    if (action == PA_ENTERARENA || action == PA_LEAVEARENA)
    {
        // ArenaData *data = P_ARENA_DATA(arena, adkey);
    }

    UNLOCK();
}

local void aaction(Arena *arena, int action)
{
    LOCK();

    if (action == AA_CREATE)
    {
        initArenaData(arena);
    }
    else if (action == AA_DESTROY)
    {
        deinitArenaData(arena);
    }

    UNLOCK();
}

EXPORT int MM_dphysics(int action, Imodman *mm_, Arena *arena)
{
    int rv = MM_FAIL;  // return value

    if (action == MM_LOAD)
    {
        mm = mm_;
        net = mm->GetInterface(I_NET, ALLARENAS);
        aman = mm->GetInterface(I_ARENAMAN, ALLARENAS);
        ml = mm->GetInterface(I_MAINLOOP, ALLARENAS);
        pd = mm->GetInterface(I_PLAYERDATA, ALLARENAS);
        log = mm->GetInterface(I_LOGMAN, ALLARENAS);

        if (!net || !aman || !ml || !pd || !log)
            return MM_FAIL;

        adkey = aman->AllocateArenaData(sizeof(ArenaData));
        if (adkey == -1)
            return MM_FAIL;

        net->AddPacket(C2S_POSITION, ppk);

        ml->SetTimer(FrameTimer, 0, 1000 / 60, NULL, NULL);

        mm->RegCallback(CB_PLAYERACTION, paction, ALLARENAS);
        mm->RegCallback(CB_ARENAACTION, aaction, ALLARENAS);

        rv = MM_OK;
    }
    else if (action == MM_UNLOAD)
    {
        mm->UnregCallback(CB_ARENAACTION, aaction, ALLARENAS);
        mm->UnregCallback(CB_PLAYERACTION, paction, ALLARENAS);

        ml->ClearTimer(FrameTimer, NULL);
        net->RemovePacket(C2S_POSITION, ppk);
        aman->FreeArenaData(adkey);

        mm->ReleaseInterface(pd);
        mm->ReleaseInterface(net);
        mm->ReleaseInterface(aman);
        mm->ReleaseInterface(ml);
        mm->ReleaseInterface(log);

        rv = MM_OK;
    }

    return rv;
}
