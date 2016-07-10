// Discretion 2 physics module

#include <assert.h>
#include <string.h>

#include "asss.h"
#include "dphysics_packets.h"
#include "packets/pdata.h"
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
    u32 frameNum;

    LinkedList pidStateList;
    LinkedList playerStateList;
    LinkedList weaponStateList;
} FrameData;

typedef struct ArenaData
{
    u32 nextFrameNum;
} ArenaData;

local void initArenaData(Arena *a)
{
    ArenaData *data = P_ARENA_DATA(a, adkey);

    data->nextFrameNum = 0;
}

local void deinitArenaData(Arena *a)
{
    // ArenaData *data = P_ARENA_DATA(a, adkey);

    // LLFree(&data->frames);
}

local void initFrameData(FrameData *fd)
{
    LLInit(&fd->pidStateList);
    LLInit(&fd->playerStateList);
    LLInit(&fd->weaponStateList);
}

local void deinitFrameData(FrameData *fd)
{
    Link *link = 0;
    void *data = 0;

    // free the pointed-to objects
    FOR_EACH(&fd->pidStateList, data, link)
    {
        afree(data);
        data = 0;
    }

    FOR_EACH(&fd->playerStateList, data, link)
    {
        afree(data);
        data = 0;
    }

    FOR_EACH(&fd->weaponStateList, data, link)
    {
        afree(data);
        data = 0;
    }

    // free the link objects
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
    header.packetType = S2C_DISC2_FRAME;
    header.frameNum = fd->frameNum;

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

    FOR_EACH(&fd->pidStateList, curPidState, link)
    {
        memcpy(curOffset, curPidState, sizeof(PidState));
        curOffset += sizeof(PidState);
    }

    // copy player state
    PlayerState *curPlayerState = 0;

    FOR_EACH(&fd->playerStateList, curPlayerState, link)
    {
        memcpy(curOffset, curPlayerState, sizeof(PlayerState));
        curOffset += sizeof(PlayerState);
    }

    // copy weapon state
    WeaponState *curWeaponState = 0;

    FOR_EACH(&fd->weaponStateList, curWeaponState, link)
    {
        memcpy(curOffset, curWeaponState, sizeof(WeaponState));
        curOffset += sizeof(WeaponState);
    }

    assert(curOffset == rv + *size);

    return rv;
}

local void sendFrameDataToAllPlayers(FrameData *fd, Arena *arena)
{
    u32 size = 0;
    byte *packet = makeFramePacket(fd, &size);
    Link *link = 0;
    Player *p = 0;

    FOR_EACH_PLAYER_IN_ARENA(p, arena)
    {
        if (p->type == T_DISC2)
            net->SendToOne(p, packet, size, NET_UNRELIABLE);
    }

    afree(packet);
}

local void populateFrameData(FrameData *fd, Arena *arena, ArenaData *arenaData)
{
    Link *link = NULL;
    Player *p = NULL;

    fd->frameNum = arenaData->nextFrameNum;

    pd->Lock();
    FOR_EACH_PLAYER_IN_ARENA(p, arena)
    {
        PidState *pid = amalloc(sizeof(PidState));

        pid->pid = p->pid;
        pid->freq = p->pkt.freq;
        pid->ship = p->pkt.ship;
        strncpy(pid->name, p->name, sizeof(pid->name));
        strncpy(pid->squad, p->squad, sizeof(pid->squad));
        LLAdd(&fd->pidStateList, pid);

        PlayerState *ps = amalloc(sizeof(PlayerState));

        ps->exploded = 0;
        ps->pid = p->pid;
        ps->rot = p->position.rotation;
        ps->xpixel = p->position.x;
        ps->ypixel = p->position.y;
        LLAdd(&fd->playerStateList, ps);
    }

    pd->Unlock();

    // need to populate weapons as well
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
        ArenaData *arenaData = P_ARENA_DATA(arena, adkey);

        populateFrameData(&fd, arena, arenaData);
        sendFrameDataToAllPlayers(&fd, arena);

        arenaData->nextFrameNum++;
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
