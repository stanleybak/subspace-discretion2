/*
 * Settings Structure from MERVBot (thanks catid!)
 * ArenaSettings *settings = &h->settings;
 * memcpy(settings, msg, 1428);
 */

#pragma once

#include "Compatibility.h"

struct ShipSettings   // 144 u8s wide, offsets are for warbird
{                     // Mostly Snrrrub
    u16 SuperTime;    // 0004 All:SuperTime:1::How long Super lasts on the ship (in hundredths of a
                      // second)
    u16 UNKNOWN0;     // 0006 (100)	Salt for actual super time?
    u16 ShieldsTime;  // 0008 All:ShieldsTime:1::How long Shields lasts on the ship (in
                      // hundredths of a second)
    u16 UNKNOWN1;     // 0010 (30)	Salt for actual shields time?
    u16 Gravity;      // 0012 All:Gravity:::Uses this formula, where R = raduis (tiles) and g = this
                      // setting; R = 1.325 * (g ^ 0.507)  IE: If set to 500, then your ship will
                      // start to get pulled in by the wormhole once you come within 31 tiles of it
    u16 GravityTopSpeed;   // 0014 All:GravityTopSpeed:::Ship are allowed to move faster than
                           // their maximum speed while effected by a wormhole.  This determines
                           // how much faster they can go (0 = no extra speed)
    u16 BulletFireEnergy;  // 0016 All:BulletFireEnergy:::Amount of energy it takes a ship to
                           // fire a single L1 bullet
    u16 MultiFireEnergy;   // 0018 All:MultiFireEnergy:::Amount of energy it takes a ship to fire
                           // multifire L1 bullets
    u16 BombFireEnergy;    // 0020 All:BombFireEnergy:::Amount of energy it takes a ship to fire a
                           // single bomb
    u16 BombFireEnergyUpgrade;  // 0022 All:BombFireEnergyUpgrade:::Extra amount of energy it
                                // takes a ship to fire an upgraded bomb. ie. L2 =
                                // BombFireEnergy+BombFireEnergyUpgrade
    u16 MineFireEnergy;         // 0024 All:LandmineFireEnergy:::Amount of energy it takes a ship to
                                // place a single L1 mine
    u16 MineFireEnergyUpgrade;  // 0026 All:LandmineFireEnergyUpgrade:::Extra amount of energy it
                                // takes to place an upgraded landmine.  ie. L2 =
                                // LandmineFireEnergy+LandmineFireEnergyUpgrade
    u16 BulletSpeed;            // 0028 All:BulletSpeed:::How fast bullets travel
    u16 BombSpeed;              // 0030 All:BombSpeed:::How fast bombs travel

    unsigned SeeBombLevel : 2;  // 0032 All:SeeBombLevel:0:4:If ship can see bombs on radar
                                // (0=Disabled, 1=All, 2=L2 and up, 3=L3 and up, 4=L4 bombs only)
                                // [Continuum .36]
    unsigned DisableFastBombs : 1;  // 0032 All:DisableFastShooting:0:1:If firing bullets, bombs, or
                                    // thors is disabled after using afterburners (1=enabled)
                                    // [Continuum .36]
    unsigned Radius : 7;  // 0032 All:Radius:::The ship's radius from center to outside, in pixels.
                          // Standard value is 14 pixels. [Continuum .37]
    unsigned
        pack0 : 6;  // 0033 Unused (fixed/updated to whatever is current by Niadh@columbus.rr.com)
    unsigned MultiFireAngle : 16;  // 0034 All:MultiFireAngle:::Angle spread between multi-fire
                                   // bullets and standard forward firing bullets. (111 = 1 degree,
                                   // 1000 = 1 ship-rotation-point)

    u16 CloakEnergy;      // 0036 All:CloakEnergy:0:32000:Amount of energy required to have 'Cloak'
                          // activated (thousanths per hundredth of a second)
    u16 StealthEnergy;    // 0038 All:StealthEnergy:0:32000:Amount of energy required to have
                          // 'Stealth' activated (thousanths per hundredth of a second)
    u16 AntiWarpEnergy;   // 0040 All:AntiWarpEnergy:0:32000:Amount of energy required to have
                          // 'Anti-Warp' activated (thousanths per hundredth of a second)
    u16 XRadarEnergy;     // 0042 All:XRadarEnergy:0:32000:Amount of energy required to have
                          // 'X-Radar' activated (thousanths per hundredth of a second)
    u16 MaximumRotation;  // 0044 All:MaximumRotation:::Maximum rotation rate of the ship (0 =
                          // can't rotate, 400 = full rotation in 1 second)
    u16 MaximumThrust;    // 0046 All:MaximumThrust:::Maximum thrust of ship (0 = none)
    u16 MaximumSpeed;     // 0048 All:MaximumSpeed:::Maximum speed of ship (0 = can't move)
    u16 MaximumRecharge;  // 0050 All:MaximumRecharge:::Maximum recharge rate, or how quickly
                          // this ship recharges its energy.
    u16 MaximumEnergy;  // 0052 All:MaximumEnergy:::Maximum amount of energy that the ship can have.
    u16 InitialRotation;  // 0054 All:InitialRotation:::Initial rotation rate of the ship (0 =
                          // can't rotate, 400 = full rotation in 1 second)
    u16 InitialThrust;    // 0056 All:InitialThrust:::Initial thrust of ship (0 = none)
    u16 InitialSpeed;     // 0058 All:InitialSpeed:::Initial speed of ship (0 = can't move)
    u16 InitialRecharge;  // 0060 All:InitialRecharge:::Initial recharge rate, or how quickly
                          // this ship recharges its energy.
    u16 InitialEnergy;  // 0062 All:InitialEnergy:::Initial amount of energy that the ship can have.
    u16 UpgradeRotation;    // 0064 All:UpgradeRotation:::Amount added per 'Rotation' Prize
    u16 UpgradeThrust;      // 0066 All:UpgradeThrust:::Amount added per 'Thruster' Prize
    u16 UpgradeSpeed;       // 0068 All:UpgradeSpeed:::Amount added per 'Speed' Prize
    u16 UpgradeRecharge;    // 0070 All:UpgradeRecharge:::Amount added per 'Recharge Rate' Prize
    u16 UpgradeEnergy;      // 0072 All:UpgradeEnergy:::Amount added per 'Energy Upgrade' Prize
    u16 AfterburnerEnergy;  // 0074 All:AfterburnerEnergy:::Amount of energy required to have
                            // 'Afterburners' activated.
    u16 BombThrust;  // 0076 All:BombThrust:::Amount of back-thrust you receive when firing a bomb.
    u16 BurstSpeed;  // 0078 All:BurstSpeed:::How fast the burst shrapnel is for this ship.
    u16 TurretThrustPenalty;  // 0080 All:TurretThrustPenalty:::Amount the ship's thrust is
                              // decreased with a turret riding
    u16 TurretSpeedPenalty;   // 0082 All:TurretSpeedPenalty:::Amount the ship's speed is
                              // decreased with a turret riding
    u16 BulletFireDelay;      // 0084 All:BulletFireDelay:::delay that ship waits after a bullet is
                              // fired until another weapon may be fired (in hundredths of a second)
    u16 MultiFireDelay;       // 0086 All:MultiFireDelay:::delay that ship waits after a multifire
    // bullet is fired until another weapon may be fired (in hundredths of a
    // second)
    u16 BombFireDelay;      // 0088 All:BombFireDelay:::delay that ship waits after a bomb is fired
                            // until another weapon may be fired (in hundredths of a second)
    u16 LandmineFireDelay;  // 0090 All:LandmineFireDelay:::delay that ship waits after a mine is
                            // fired until another weapon may be fired (in hundredths of a
                            // second)
    u16 RocketTime;     // 0092 All:RocketTime:::How long a Rocket lasts (in hundredths of a second)
    u16 InitialBounty;  // 0094 All:InitialBounty:::Number of 'Greens' given to ships when they
                        // start
    u16 DamageFactor;   // 0096 All:DamageFactor:::How likely a the ship is to take damamage (ie.
                        // lose a prize) (0=special-case-never, 1=extremely likely, 5000=almost
                        // never)
    u16 PrizeShareLimit;  // 0098 All:PrizeShareLimit:::Maximum bounty that ships receive Team
                          // Prizes
    u16 AttachBounty;     // 0100 All:AttachBounty:::Bounty required by ships to attach as a turret
    u16 SoccerThrowTime;  // 0102 All:SoccerThrowTime:::Time player has to carry soccer ball (in
                          // hundredths of a second)
    u16 SoccerBallFriction;   // 0104 All:SoccerBallFriction:::Amount the friction on the soccer
                              // ball (how quickly it slows down -- higher numbers mean faster
                              // slowdown)
    u16 SoccerBallProximity;  // 0106 All:SoccerBallProximity:::How close the player must be in
                              // order to pick up ball (in pixels)
    u16 SoccerBallSpeed;      // 0108 All:SoccerBallSpeed:::Initial speed given to the ball when
                              // fired by the carrier.

    u8 TurretLimit;    // 0110 All:TurretLimit:::Number of turrets allowed on a ship.
    u8 BurstShrapnel;  // 0111 All:BurstShrapnel:::Number of bullets released when a 'Burst' is
                       // activated
    u8 MaxMines;       // 0112 All:MaxMines:::Maximum number of mines allowed in ships
    u8 RepelMax;       // 0113 All:RepelMax:::Maximum number of Repels allowed in ships
    u8 BurstMax;       // 0114 All:BurstMax:::Maximum number of Bursts allowed in ships
    u8 DecoyMax;       // 0115 All:DecoyMax:::Maximum number of Decoys allowed in ships
    u8 ThorMax;        // 0116 All:ThorMax:::Maximum number of Thor's Hammers allowed in ships
    u8 BrickMax;       // 0117 All:BrickMax:::Maximum number of Bricks allowed in ships
    u8 RocketMax;      // 0118 All:RocketMax:::Maximum number of Rockets allowed in ships
    u8 PortalMax;      // 0119 All:PortalMax:::Maximum number of Portals allowed in ships
    u8 InitialRepel;   // 0120 All:InitialRepel:::Initial number of Repels given to ships when they
                       // start
    u8 InitialBurst;   // 0121 All:InitialBurst:::Initial number of Bursts given to ships when they
                       // start
    u8 InitialBrick;   // 0122 All:InitialBrick:::Initial number of Bricks given to ships when they
                       // start
    u8 InitialRocket;  // 0123 All:InitialRocket:::Initial number of Rockets given to ships when
                       // they start
    u8 InitialThor;    // 0124 All:InitialThor:::Initial number of Thor's Hammers given to ships
                       // when they start
    u8 InitialDecoy;   // 0125 All:InitialDecoy:::Initial number of Decoys given to ships when they
                       // start
    u8 InitialPortal;  // 0126 All:InitialPortal:::Initial number of Portals given to ships when
                       // they start
    u8 BombBounceCount;  // 0127 All:BombBounceCount:::Number of times a ship's bombs bounce
                         // before they explode on impact

    unsigned ShrapnelMax : 5;  // 0128 All:ShrapnelMax:0:31:Maximum amount of shrapnel released from
                               // a ship's bomb
    unsigned ShrapnelRate : 5;  // 0128 All:ShrapnelRate:0:31:Amount of additional shrapnel gained
                                // by a 'Shrapnel Upgrade' prize.

    unsigned CloakStatus : 2;     // 0129 All:CloakStatus:0:2:Whether ships are allowed to receive
                                  // 'Cloak' 0=no 1=yes 2=yes/start-with
    unsigned StealthStatus : 2;   // 0129 All:StealthStatus:0:2:Whether ships are allowed to receive
                                  // 'Stealth' 0=no 1=yes 2=yes/start-with
    unsigned XRadarStatus : 2;    // 0129 All:XRadarStatus:0:2:Whether ships are allowed to receive
                                  // 'X-Radar' 0=no 1=yes 2=yes/start-with
    unsigned AntiwarpStatus : 2;  // 0130 All:AntiWarpStatus:0:2:Whether ships are allowed to
                                  // receive 'Anti-Warp' 0=no 1=yes 2=yes/start-with

    unsigned
        InitialGuns : 2;   // 0130 All:InitialGuns:0:3:Initial level a ship's guns fire 0=no guns
    unsigned MaxGuns : 2;  // 0130 All:MaxGuns:0:3:Maximum level a ship's guns can fire 0=no guns
    unsigned
        InitialBombs : 2;  // 0130 All:InitialBombs:0:3:Initial level a ship's bombs fire 0=no bombs
    unsigned
        MaxBombs : 2;  // 0131 All:MaxBombs:0:3:Maximum level a ship's bombs can fire 0=no bombs

    unsigned DoubleBarrel : 1;  // 0131 All:DoubleBarrel:0:1:Whether ships fire with double barrel
                                // bullets 0=no 1=yes
    unsigned EmpBomb : 1;       // 0131 All:EmpBomb:0:1:Whether ships fire EMP bombs 0=no 1=yes
    unsigned SeeMines : 1;      // 0131 All:SeeMines:0:1:Whether ships see mines on radar 0=no 1=yes
    unsigned UNKNOWN2 : 3;      // 0131 ?

    u8 UNKNOWN3[16];  // 0132 ?
};

// Prize settings

struct PrizeSettings     // 28 u8s wide
{                        // All Snrrrub
    u8 Recharge;         // 1400 [fixed] PrizeWeight:QuickCharge:::Likelyhood of 'Recharge' prize
                         // appearing
    u8 Energy;           // 1401 PrizeWeight:Energy:::Likelyhood of 'Energy Upgrade' prize appearing
    u8 Rotation;         // 1402 PrizeWeight:Rotation:::Likelyhood of 'Rotation' prize appearing
    u8 Stealth;          // 1403 PrizeWeight:Stealth:::Likelyhood of 'Stealth' prize appearing
    u8 Cloak;            // 1404 PrizeWeight:Cloak:::Likelyhood of 'Cloak' prize appearing
    u8 XRadar;           // 1405 PrizeWeight:XRadar:::Likelyhood of 'XRadar' prize appearing
    u8 Warp;             // 1406 PrizeWeight:Warp:::Likelyhood of 'Warp' prize appearing
    u8 Gun;              // 1407 PrizeWeight:Gun:::Likelyhood of 'Gun Upgrade' prize appearing
    u8 Bomb;             // 1408 PrizeWeight:Bomb:::Likelyhood of 'Bomb Upgrade' prize appearing
    u8 BouncingBullets;  // 1409 PrizeWeight:BouncingBullets:::Likelyhood of 'Bouncing Bullets'
                         // prize appearing
    u8 Thruster;         // 1410 PrizeWeight:Thruster:::Likelyhood of 'Thruster' prize appearing
    u8 TopSpeed;         // 1411 PrizeWeight:TopSpeed:::Likelyhood of 'Speed' prize appearing
    u8 QuickCharge;      // 1412 [fixed] PrizeWeight:Recharge:::Likelyhood of 'Full Charge' prize
                         // appearing (NOTE! This is FULL CHARGE, not Recharge!! stupid vie)
    u8 Glue;             // 1413 PrizeWeight:Glue:::Likelyhood of 'Engine Shutdown' prize appearing
    u8 MultiFire;        // 1414 PrizeWeight:MultiFire:::Likelyhood of 'MultiFire' prize appearing
    u8 Proximity;   // 1415 PrizeWeight:Proximity:::Likelyhood of 'Proximity Bomb' prize appearing
    u8 AllWeapons;  // 1416 PrizeWeight:AllWeapons:::Likelyhood of 'Super!' prize appearing
    u8 Shields;     // 1417 PrizeWeight:Shields:::Likelyhood of 'Shields' prize appearing
    u8 Shrapnel;    // 1418 PrizeWeight:Shrapnel:::Likelyhood of 'Shrapnel Upgrade' prize appearing
    u8 AntiWarp;    // 1419 PrizeWeight:AntiWarp:::Likelyhood of 'AntiWarp' prize appearing
    u8 Repel;       // 1420 PrizeWeight:Repel:::Likelyhood of 'Repel' prize appearing
    u8 Burst;       // 1421 PrizeWeight:Burst:::Likelyhood of 'Burst' prize appearing
    u8 Decoy;       // 1422 PrizeWeight:Decoy:::Likelyhood of 'Decoy' prize appearing
    u8 Thor;        // 1423 PrizeWeight:Thor:::Likelyhood of 'Thor' prize appearing
    u8 MultiPrize;  // 1424 PrizeWeight:MultiPrize:::Likelyhood of 'Multi-Prize' prize appearing
    u8 Brick;       // 1425 PrizeWeight:Brick:::Likelyhood of 'Brick' prize appearing
    u8 Rocket;      // 1426 PrizeWeight:Rocket:::Likelyhood of 'Rocket' prize appearing
    u8 Portal;      // 1427 PrizeWeight:Portal:::Likelyhood of 'Portal' prize appearing
};

// Arena settings

struct ArenaSettings           // 1428 u8s wide
{                              // Initial template by Snrrrub
    unsigned Version : 8;      // 0000 VIE:15
    unsigned ExactDamage : 1;  // 0001 Bullet:ExactDamage:0:1:If damage is to be random or not
                               // (1=exact, 0=random) [Continuum .36]
    unsigned HideFlags : 1;  // 0001 Spectator:HideFlags:0:1:If flags are to be shown to specs when
                             // they are dropped (1=can't see them) [Continuum .36]
    unsigned NoXRadar : 1;   // 0001 Spectator:NoXRadar:0:1:If specs are allowed to have X (0=yes,
                             // 1=no) [Continuum .36]
    unsigned pack0 : 21;     // 0001 (14)

    ShipSettings ships[8];  // 0004 See shipSettings declaration...

    u32 BulletDamageLevel;  // 1156 [BulletDamageLevel * 1000] Bullet:BulletDamageLevel:::Maximum
                            // amount of damage that a L1 bullet will cause. Formula; damage =
                            // squareroot(rand# * (max damage^2 + 1))
    u32 BombDamageLevel;    // 1160 [BombDamageLevel * 1000] Bomb:BombDamageLevel:::Amount of
                            // damage a bomb causes at its center point (for all bomb levels)
    u32 BulletAliveTime;    // 1164 Bullet:BulletAliveTime:::How long bullets live before
                            // disappearing (in hundredths of a second)
    u32 BombAliveTime;   // 1168 Bomb:BombAliveTime:::Time bomb is alive (in hundredths of a second)
    u32 DecoyAliveTime;  // 1172 Misc:DecoyAliveTime:::Time a decoy is alive (in hundredths of a
                         // second)
    u32 SafetyLimit;     // 1176 Misc:SafetyLimit:::Amount of time that can be spent in the safe
                         // zone. (90000 = 15 mins)
    u32 FrequencyShift;  // 1180 Misc:FrequencyShift:0:10000:Amount of random frequency shift
                         // applied to sounds in the game.
    u32 MaxFrequency;    // 1184 Team:MaxFrequency:::Maximum number of frequencies allowed in arena
                         // (5 would allow frequencies 0,1,2,3,4)
    u32 RepelSpeed;      // 1188 Repel:RepelSpeed:::Speed at which players are repelled
    u32 MineAliveTime;   // 1192 Mine:MineAliveTime:0:60000:Time that mines are active (in
                         // hundredths of a second)
    u32 BurstDamageLevel;        // 1196 [BurstDamageLevel * 1000] Burst:BurstDamageLevel:::Maximum
                                 // amount of damage caused by a single burst bullet.
    u32 BulletDamageUpgrade;     // 1200 [BulletDamageUpgrade * 1000]
                                 // Bullet:BulletDamageUpgrade:::Amount of extra damage each bullet
                                 // level will cause
    u32 FlagDropDelay;           // 1204 Flag:FlagDropDelay:::Time before flag is dropped by carrier
                                 // (0=never)
    u32 EnterGameFlaggingDelay;  // 1208 Flag:EnterGameFlaggingDelay:::Time a new player must
                                 // wait before they are allowed to see flags
    u32 RocketThrust;  // 1212 Rocket:RocketThrust:::Thrust value given while a rocket is active.
    u32 RocketSpeed;   // 1216 Rocket:RocketSpeed:::Speed value given while a rocket is active.
    u32 InactiveShrapnelDamage;   // 1220 [InactiveShrapnelDamage * 1000]
                                  // Shrapnel:InactiveShrapDamage:::Amount of damage shrapnel
                                  // causes in it's first 1/4 second of life.
    u32 WormholeSwitchTime;       // 1224 Wormhole:SwitchTime:::How often the wormhole switches its
                                  // destination.
    u32 ActivateAppShutdownTime;  // 1228 Misc:ActivateAppShutdownTime:::Amount of time a ship is
                                  // shutdown after application is reactivated (ie. when they
                                  // come back from windows mode)
    u32 ShrapnelSpeed;            // 1232 Shrapnel:ShrapnelSpeed:::Speed that shrapnel travels

    u8 UNKNOWN0[16];  // 1236 ?

    u16 SendRoutePercent;   // 1252 Latency:SendRoutePercent:300:800:Percentage of the ping time
                            // that is spent on the ClientToServer portion of the ping. (used in
                            // more accurately syncronizing clocks)
    u16 BombExplodeDelay;   // 1254 Bomb:BombExplodeDelay:::How long after the proximity sensor is
                            // triggered before bomb explodes. (note: it explodes immediately if
                            // ship moves away from it after triggering it)
    u16 SendPositionDelay;  // 1256 Misc:SendPositionDelay:0:20:Amount of time between position
                            // packets sent by client.
    u16 BombExplodePixels;  // 1258 Bomb:BombExplodePixels:::Blast radius in pixels for an L1
                            // bomb (L2 bombs double this, L3 bombs triple this)
    u16 DeathPrizeTime;     // 1260 Prize:DeathPrizeTime:::How long the prize exists that appears
                            // after killing somebody.
    u16 JitterTime;  // 1262 Bomb:JitterTime:::How long the screen jitters from a bomb hit. (in
                     // hundredths of a second)
    u16 EnterDelay;  // 1264 Kill:EnterDelay:::How long after a player dies before he can
                     // re-enter the game.
    u16 EngineShutdownTime;     // 1266 Prize:EngineShutdownTime:::Time the player is affected by an
                                // 'Engine Shutdown' Prize (in hundredth of a second)
    u16 ProximityDistance;      // 1268 Bomb:ProximityDistance:::Radius of proximity trigger in
                                // tiles.  Each bomb level adds 1 to this amount.
    u16 BountyIncreaseForKill;  // 1270 Kill:BountyIncreaseForKill:::Number of points added to
                                // players bounty each time he kills an opponent.
    u16 BounceFactor;   // 1272 Misc:BounceFactor:::How bouncy the walls are (16=no-speed-loss)
    u16 MapZoomFactor;  // 1274 Radar:MapZoomFactor:8:1000:A number representing how far you can
                        // see on radar.
    u16 MaxBonus;       // 1276 Kill:MaxBonus:::Let's ignore these for now. Or let's not. :) This is
                        // if you have flags, can add more points per a kill. Founded by MGB
    u16 MaxPenalty;     // 1278 Kill:MaxPenalty:::Let's ignore these for now. Or let's not. :) This
                        // is if you have flags, can take away points per a kill. Founded by MGB
    u16 RewardBase;     // 1280 Kill:RewardBase:::Let's ignore these for now. Or let's not. :) This
                        // is shown added to a person's bty, but isn't added from points for a kill.
                        // Founded by MGB
    u16 RepelTime;      // 1282 Repel:RepelTime:::Time players are affected by the repel (in
                        // hundredths of a second)
    u16 RepelDistance;  // 1284 Repel:RepelDistance:::Number of pixels from the player that are
                        // affected by a repel.
    u16 HelpTickerDelay;  // 1286 Misc:TickerDelay:::Amount of time between ticker help messages.
    u16 FlaggerOnRadar;   // 1288 Flag:FlaggerOnRadar:::Whether the flaggers appear on radar in
                          // red 0=no 1=yes
    u16 FlaggerKillMultiplier;  // 1290 Flag:FlaggerKillMultiplier:::Number of times more points
                                // are given to a flagger (1 = double points, 2 = triple points)
    u16 PrizeFactor;  // 1292 Prize:PrizeFactor:::Number of prizes hidden is based on number of
                      // players in game.  This number adjusts the formula, higher numbers mean
                      // more prizes. (*Note: 10000 is max, 10 greens per person)
    u16 PrizeDelay;   // 1294 Prize:PrizeDelay:::How often prizes are regenerated (in hundredths
                      // of a second)
    u16 PrizeMinimumVirtual;  // 1296 Prize:MinimumVirtual:::Distance from center of arena that
                              // prizes/flags/soccer-balls will generate
    u16 PrizeUpgradeVirtual;  // 1298 Prize:UpgradeVirtual:::Amount of additional distance added
                              // to MinimumVirtual for each player that is in the game.
    u16 PrizeMaxExist;  // 1300 Prize:PrizeMaxExist:::Maximum amount of time that a hidden prize
                        // will remain on screen. (actual time is random)
    u16 PrizeMinExist;  // 1302 Prize:PrizeMinExist:::Minimum amount of time that a hidden prize
                        // will remain on screen. (actual time is random)
    u16 PrizeNegativeFactor;  // 1304 Prize:PrizeNegativeFactor:::Odds of getting a negative
                              // prize.  (1 = every prize, 32000 = extremely rare)
    u16 DoorDelay;       // 1306 Door:DoorDelay:::How often doors attempt to switch their state.
    u16 AntiwarpPixels;  // 1308 Toggle:AntiWarpPixels:::Distance Anti-Warp affects other players
                         // (in pixels) (note: enemy must also be on radar)
    i16 DoorMode;  // 1310 Door:DoorMode:::Door mode (-2=all doors completely random, -1=weighted
                   // random (some doors open more often than others), 0-255=fixed doors (1 bit
                   // of u8 for each door specifying whether it is open or not)
    u16 FlagBlankDelay;  // 1312 Flag:FlagBlankDelay:::Amount of time that a user can get no data
                         // from server before flags are hidden from view for 10 seconds.
    u16 NoDataFlagDropDelay;  // 1314 Flag:NoDataFlagDropDelay:::Amount of time that a user can
                              // get no data from server before flags he is carrying are dropped.
    u16 MultiPrizeCount;      // 1316 Prize:MultiPrizeCount:::Number of random 'Greens' given with a
                              // 'MultiPrize'
    u16 BrickTime;        // 1318 Brick:BrickTime:::How long bricks last (in hundredths of a second)
    u16 WarpRadiusLimit;  // 1320 Misc:WarpRadiusLimit:::When ships are randomly placed in the
                          // arena, this parameter will limit how far from the center of the
                          // arena they can be placed (1024=anywhere)
    u16 EBombShutdownTime;   // 1322 Bomb:EBombShutdownTime:::Maximum time recharge is stopped on
                             // players hit with an EMP bomb.
    u16 EBombDamagePercent;  // 1324 Bomb:EBombDamagePercent:::Percentage of normal damage
                             // applied to an EMP bomb 0=0% 1000=100% 2000=200%
    u16 RadarNeutralSize;    // 1326 Radar:RadarNeutralSize:0:1024:Size of area between blinded
                             // radar zones (in pixels)
    u16 WarpPointDelay;      // 1328 Misc:WarpPointDelay:::How long a Portal point is active.
    u16 NearDeathLevel;      // 1330 Misc:NearDeathLevel:::Amount of energy that constitutes a
    // near-death experience (ships bounty will be decreased by 1 when this
    // occurs -- used for dueling zone)
    u16 BBombDamagePercent;      // 1332 Bomb:BBombDamagePercent:::Percentage of normal damage
                                 // applied to a bouncing bomb 0=0% 1000=100% 2000=200%
    u16 ShrapnelDamagePercent;   // 1334 Shrapnel:ShrapnelDamagePercent:::Percentage of normal
                                 // damage applied to shrapnel (relative to bullets of same level)
                                 // 0=0% 1000=100% 2000=200%
    u16 ClientSlowPacketTime;    // 1336 Latency:ClientSlowPacketTime:20:200:Amount of latency S2C
                                 // that constitutes a slow packet.
    u16 FlagDropResetReward;     // 1338 Flag:FlagDropResetReward:::Minimum kill reward that a
                                 // player must get in order to have his flag drop timer reset.
    u16 FlaggerFireCostPercent;  // 1340 Flag:FlaggerFireCostPercent:::Percentage of normal
                                 // weapon firing cost for flaggers 0=Super 1000=100% 2000=200%
    u16 FlaggerDamagePercent;    // 1342 Flag:FlaggerDamagePercent:::Percentage of normal damage
                                 // received by flaggers 0=Invincible 1000=100% 2000=200%
    u16 FlaggerBombFireDelay;    // 1344 Flag:FlaggerBombFireDelay:::Delay given to flaggers for
    // firing bombs (0=ships normal firing rate -- note: please do not
    // set this number less than 20)
    u16 SoccerPassDelay;          // 1346 Soccer:PassDelay:0:10000:How long after the ball is fired
                                  // before anybody can pick it up (in hundredths of a second)
    u16 SoccerBallBlankDelay;     // 1348 Soccer:BallBlankDelay:::Amount of time a player can
                                  // receive no data from server and still pick up the soccer ball.
    u16 S2CNoDataKickoutDelay;    // 1350 Latency:S2CNoDataKickoutDelay:100:32000:Amount of time a
                                  // user can receive no data from server before connection is
                                  // terminated.
    u16 FlaggerThrustAdjustment;  // 1352 Flag:FlaggerThrustAdjustment:::Amount of thrust
                                  // adjustment player carrying flag gets (negative numbers mean
                                  // less thrust)
    u16 FlaggerSpeedAdjustment;   // 1354 Flag:FlaggerSpeedAdjustment:::Amount of speed adjustment
                                  // player carrying flag gets (negative numbers mean slower)
    u16 CliSlowPacketSampleSize;  // 1356 Latency:ClientSlowPacketSampleSize:50:1000:Number of
                                  // packets to sample S2C before checking for kickout.

    u8 UNKNOWN1[10];  // 1358 ?

    u8 RandomShrapnel;    // 1368 Shrapnel:Random:0:1:Whether shrapnel spreads in circular or random
                          // patterns 0=circular 1=random
    u8 SoccerBallBounce;  // 1369 Soccer:BallBounce:0:1:Whether the ball bounces off walls (0=ball
                          // go through walls, 1=ball bounces off walls)
    u8 SoccerAllowBombs;  // 1370 Soccer:AllowBombs:0:1:Whether the ball carrier can fire his
                          // bombs (0=no 1=yes)
    u8 SoccerAllowGuns;   // 1371 Soccer:AllowGuns:0:1:Whether the ball carrier can fire his guns
                          // (0=no 1=yes)
    u8 SoccerMode;        // 1372 Soccer:Mode:0:6:Goal configuration (0=any goal,
                          // 1=left-half/right-half, 2=top-half/bottom-half,
                          // 3=quadrants-defend-one-goal, 4=quadrants-defend-three-goals,
                          // 5=sides-defend-one-goal, 6=sides-defend-three-goals)
    u8 MaxPerTeam;  // 1373 Team:MaxPerTeam:::Maximum number of players on a non-private frequency
    u8 MaxPerPrivateTeam;  // 1374 Team:MaxPerPrivateTeam:::Maximum number of players on a private
                           // frequency (0=same as MaxPerTeam)
    u8 TeamMaxMines;       // 1375 Mine:TeamMaxMines:0:32000:Maximum number of mines allowed to be
                           // placed by an entire team
    u8 WormholeGravityBombs;  // 1376 Wormhole:GravityBombs:0:1:Whether a wormhole affects bombs
                              // (0=no 1=yes)
    u8 BombSafety;  // 1377 Bomb:BombSafety:0:1:Whether proximity bombs have a firing safety (0=no
                    // 1=yes).  If enemy ship is within proximity radius, will it allow you to
                    // fire.
    u8 MessageReliable;     // 1378 Message:MessageReliable:0:1:Whether messages are sent reliably.
    u8 TakePrizeReliable;   // 1379 Prize:TakePrizeReliable:0:1:Whether prize packets are sent
                            // reliably (C2S)
    u8 AllowAudioMessages;  // 1380 Message:AllowAudioMessages:0:1:Whether players can send audio
                            // messages (0=no 1=yes)
    u8 PrizeHideCount;  // 1381 Prize:PrizeHideCount:::Number of prizes that are regenerated every
                        // PrizeDelay.
    u8 ExtraPositionData;  // 1382 Misc:ExtraPositionData:0:1:Whether regular players receive
                           // sysop data about a ship (leave this at zero)
    u8 SlowFrameCheck;     // 1383 Misc:SlowFrameCheck:0:1:Whether to check for slow frames on the
    // client (possible cheat technique) (flawed on some machines, do not use)
    u8 CarryFlags;      // 1384 Flag:CarryFlags:0:2:Whether the flags can be picked up and carried
                        // (0=no, 1=yes, 2=yes-one at a time)
    u8 AllowSavedShip;  // 1385 Misc:AllowSavedShips:0:1:Whether saved ships are allowed (do not
                        // allow saved ship in zones where sub-arenas may have differing
                        // parameters) 1 = Savedfrom last arena/lagout, 0 = New Ship when entering
                        // arena/zone
    u8 RadarMode;       // 1386 Radar:RadarMode:0:4:Radar mode (0=normal, 1=half/half, 2=quarters,
                        // 3=half/half-see team mates, 4=quarters-see team mates)
    u8 VictoryMusic;    // 1387 Misc:VictoryMusic:0:1:Whether the zone plays victory music or not.
    u8 FlaggerGunUpgrade;   // 1388 Flag:FlaggerGunUpgrade:0:1:Whether the flaggers get a gun
                            // upgrade 0=no 1=yes
    u8 FlaggerBombUpgrade;  // 1389 Flag:FlaggerBombUpgrade:0:1:Whether the flaggers get a bomb
                            // upgrade 0=no 1=yes
    u8 SoccerUseFlagger;    // 1390 Soccer:UseFlagger:0:1:If player with soccer ball should use the
                            // Flag:Flagger* ship adjustments or not (0=no, 1=yes)
    u8 SoccerBallLocation;  // 1391 Soccer:BallLocation:0:1:Whether the balls location is
                            // displayed at all times or not (0=not, 1=yes)

    u8 UNKNOWN2[8];  // 1392 ?

    PrizeSettings pw;  // 1400 See prizeSettings declaration...
};
