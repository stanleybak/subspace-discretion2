/*
 * Discretion 2 physics packets header, Stanley Bak, July 2016
 *
 * Frames are a snapshot of the state of the game. They are sent 60 times a second.
 *
 */

#ifndef DPHYSICS_PACKETS_H_
#define DPHYSICS_PACKETS_H_

#include "asss.h"

#pragma pack(push, 1)

#define S2C_DISC2_FRAME 0xD0

// Frame Header is the beginning of a frame packet
// It is followed by a variable number of other structs, in the same order as in the header
typedef struct FrameHeader
{
    u16 numPidStates;
    u16 numPlayerStates;
    u16 numWeaponStates;
} FrameHeader;

typedef struct PidState
{
    u16 pid;  // new pid
    char name[24];
    char squad[24];
    unsigned ship : 4;   // 0-8 (including spec)
    unsigned freq : 10;  // 0-7
} PidState;

typedef struct PlayerState
{
    u16 pid;
    unsigned rot : 6;      // 0-39
    unsigned xpixel : 14;  // 0-1024 * 16
    unsigned ypixel : 14;  // 0-1024 * 16
    unsigned exploded : 1;
} PlayerState;

typedef struct WeaponState
{
    unsigned type : 4;     // one of the W_ constants in ppk.h
    unsigned xpixel : 14;  // 0-1024 * 16
    unsigned ypixel : 14;  // 0-1024 * 16
    unsigned exploded : 1;
} WeaponState;

#pragma pack(pop)

#endif
