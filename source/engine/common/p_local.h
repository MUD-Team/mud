// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 5dd044a0308f90a62cb45eab2fb8f1f2fe9b0575 $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
// Copyright (C) 2024 by The MUD Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Play functions, animation, global header.
//
//-----------------------------------------------------------------------------

#pragma once

#include <set>

#include "d_player.h"

#define FLOATSPEED (FRACUNIT * 4)

#define STEEPSLOPE 46341 // [RH] Minimum floorplane.c value for walking

#define MAXHEALTH  100
#define VIEWHEIGHT (41 * FRACUNIT)

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS 128
#define MAPBLOCKSIZE  (MAPBLOCKUNITS * FRACUNIT)
#define MAPBLOCKSHIFT (FRACBITS + 7)
#define MAPBMASK      (MAPBLOCKSIZE - 1)
#define MAPBTOFRAC    (MAPBLOCKSHIFT - FRACBITS)

// player radius for movement checking
#define PLAYERRADIUS 16 * FRACUNIT

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
#define MAXRADIUS 32 * FRACUNIT

// #define GRAVITY 		FRACUNIT
#define MAXMOVE (30 * FRACUNIT)

#define STOPSPEED 0x1000
#define FRICTION  0xe800

#define USERANGE     (64 * FRACUNIT)
#define MELEERANGE   (64 * FRACUNIT)
#define MISSILERANGE (32 * 64 * FRACUNIT)

#define WATER_SINK_FACTOR       3
#define WATER_SINK_SMALL_FACTOR 4
#define WATER_SINK_SPEED        (FRACUNIT / 2)
#define WATER_JUMP_SPEED        (FRACUNIT * 7 / 2)

// follow a player exlusively for 3 seconds
#define BASETHRESHOLD 100

//
// P_PSPR
//

enum weaponstate_t
{
    upstate,
    downstate,
    readystate,
    atkstate,
    unknownstate
};

void P_SetupPsprites(player_t *curplayer);
void P_MovePsprites(player_t *curplayer);
void P_DropWeapon(player_t *player);

weaponstate_t P_GetWeaponState(player_t *player);

//
// P_USER
//
void P_FallingDamage(AActor *ent);
void P_PlayerThink(player_t *player);
bool P_AreTeammates(player_t &a, player_t &b);
bool P_CanSpy(player_t &viewer, player_t &other);

//
// P_MOBJ
//
#define ONFLOORZ   INT32_MIN
#define ONCEILINGZ INT32_MAX

// Time interval for item respawning.
#define ITEMQUESIZE 128

extern mapthing2_t itemrespawnque[ITEMQUESIZE];
extern int32_t     itemrespawntime[ITEMQUESIZE];
extern int32_t     iquehead;
extern int32_t     iquetail;

void P_ThrustMobj(AActor *mo, angle_t angle, fixed_t move);
void P_RespawnSpecials(void);

bool P_SetMobjState(AActor *mobj, statenum_t state, bool cl_update = false);

void    P_SpawnBlood(fixed_t x, fixed_t y, fixed_t z, int32_t damage);
AActor *P_SpawnMissile(AActor *source, AActor *dest, mobjtype_t type);
void    P_SpawnPlayerMissile(AActor *source, mobjtype_t type);
void    P_SpawnMBF21PlayerMissile(AActor *source, mobjtype_t type, fixed_t angle, fixed_t pitch, fixed_t xyofs,
                                  fixed_t zofs);
bool    P_CheckSwitchWeapon(player_t *player, weapontype_t weapon);

void P_RailAttack(AActor *source, int32_t damage, int32_t offset); // [RH] Shoot a railgun
bool P_HitFloor(AActor *thing);
//
// [RH] P_THINGS
//
extern int32_t       SpawnableThings[];
extern const int32_t NumSpawnableThings;

bool P_Thing_Spawn(int32_t tid, int32_t type, angle_t angle, bool fog);
bool P_Thing_Projectile(int32_t tid, int32_t type, angle_t angle, fixed_t speed, fixed_t vspeed, bool gravity);
bool P_ActivateMobj(AActor *mobj, AActor *activator);
bool P_DeactivateMobj(AActor *mobj);

//
// P_ENEMY
//
void P_NoiseAlert(AActor *target, AActor *emmiter);
void P_SpawnBrainTargets(void); // killough 3/26/98: spawn icon landings

extern struct brain_s
{                               // killough 3/26/98: global state of boss brain
    int32_t easy, targeton;
} brain;

//
// P_MAPUTL
//
typedef struct
{
    fixed_t x;
    fixed_t y;
    fixed_t dx;
    fixed_t dy;

} divline_t;

typedef struct
{
    fixed_t frac; // along trace line
    bool    isaline;
    union {
        AActor *thing;
        line_t *line;
    } d;
} intercept_t;

#define INT32_MAXERCEPTS 128

extern TArray<intercept_t> intercepts;

typedef bool (*traverser_t)(intercept_t *in);

subsector_t *P_PointInSubsector(fixed_t x, fixed_t y);
fixed_t      P_AproxDistance(fixed_t dx, fixed_t dy);
fixed_t      P_AproxDistance2(fixed_t *pos_array, fixed_t x, fixed_t y);
fixed_t      P_AproxDistance2(AActor *mo, fixed_t x, fixed_t y);
fixed_t      P_AproxDistance2(AActor *a, AActor *b);

bool    P_ActorInFOV(AActor *origin, AActor *mo, float f, fixed_t dist);
AActor *P_RoughTargetSearch(AActor *mo, angle_t fov, int32_t distance);

int32_t P_PointOnLineSide(fixed_t x, fixed_t y, const line_t *line);
int32_t P_PointOnDivlineSide(fixed_t x, fixed_t y, const divline_t *line);
void    P_MakeDivline(const line_t *li, divline_t *dl);
fixed_t P_InterceptVector(const divline_t *v2, const divline_t *v1);
int32_t P_BoxOnLineSide(const fixed_t *tmbox, const line_t *ld);

extern fixed_t opentop;
extern fixed_t openbottom;
extern fixed_t openrange;
extern fixed_t lowfloor;

void P_LineOpening(const line_t *linedef, fixed_t x, fixed_t y, fixed_t refx = INT32_MIN, fixed_t refy = 0);

bool P_BlockLinesIterator(int32_t x, int32_t y, bool (*func)(line_t *));
bool P_BlockThingsIterator(int32_t x, int32_t y, bool (*func)(AActor *), AActor *start = NULL);

#define PT_ADDLINES  1
#define PT_ADDTHINGS 2
#define PT_EARLYOUT  4

extern divline_t trace;

bool P_PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, int32_t flags, bool (*trav)(intercept_t *));

// [ML] 2/1/10: Break out P_PointToAngle from R_PointToAngle2 (from EE)
angle_t P_PointToAngle(fixed_t xo, fixed_t yo, fixed_t x, fixed_t y);

//
// P_MAP
//

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern bool        floatok;
extern fixed_t     tmfloorz;
extern fixed_t     tmceilingz;
extern msecnode_t *sector_list;  // phares 3/16/98
extern AActor     *BlockingMobj;
extern line_t     *BlockingLine; // Used only by P_Move
                                 // This is not necessarily a *blocking* line

extern fixed_t   tmdropoffz;     // Needed in b_move.c
extern sector_t *tmfloorsector;

extern line_t *ceilingline;

void    P_TestActorMovement(AActor *mo, fixed_t tryx, fixed_t tryy, fixed_t tryz, fixed_t &destx, fixed_t &desty,
                            fixed_t &destz);
bool    P_TestMobjZ(AActor *actor);
bool    P_TestMobjLocation(AActor *mobj);
bool    P_CheckPosition(AActor *thing, fixed_t x, fixed_t y);
AActor *P_CheckOnmobj(AActor *thing);
void    P_FakeZMovement(AActor *mo);
bool    P_CheckSlopeWalk(AActor *actor, fixed_t &xmove, fixed_t &ymove);
bool    P_TryMove(AActor *thing, fixed_t x, fixed_t y, bool dropoff, bool onfloor = false);
bool    P_TeleportMove(AActor *thing, fixed_t x, fixed_t y, fixed_t z,
                       bool telefrag); // [RH] Added z and telefrag parameters
void    P_SlideMove(AActor *mo);
bool    P_CheckSight(const AActor *t1, const AActor *t2);
void    P_UseLines(player_t *player);
void    P_ApplyTorque(AActor *mo);
void    P_CopySector(sector_t *dest, sector_t *src);

fixed_t P_PlaneZ(fixed_t x, fixed_t y, const plane_t *plane);
double  P_PlaneZ(double x, double y, const plane_t *plane);
fixed_t P_FloorHeight(fixed_t x, fixed_t y, const sector_t *sec = NULL);
fixed_t P_FloorHeight(const AActor *mo);
fixed_t P_FloorHeight(const sector_t *sector);
fixed_t P_CeilingHeight(fixed_t x, fixed_t y, const sector_t *sec = NULL);
fixed_t P_CeilingHeight(const AActor *mo);
fixed_t P_CeilingHeight(const sector_t *sector);
fixed_t P_LowestHeightOfCeiling(sector_t *sector);
fixed_t P_LowestHeightOfFloor(sector_t *sector);
fixed_t P_HighestHeightOfCeiling(sector_t *sector);
fixed_t P_HighestHeightOfFloor(sector_t *sector);

bool P_IsPlaneLevel(const plane_t *plane);
bool P_IdenticalPlanes(const plane_t *pl1, const plane_t *pl2);
void P_InvertPlane(plane_t *plane);
void P_ChangeCeilingHeight(sector_t *sector, fixed_t amount);
void P_ChangeFloorHeight(sector_t *sector, fixed_t amount);
void P_SetCeilingHeight(sector_t *sector, fixed_t value);
void P_SetFloorHeight(sector_t *sector, fixed_t value);
bool P_PointOnPlane(const plane_t *plane, fixed_t x, fixed_t y, fixed_t z);
bool P_PointAbovePlane(const plane_t *plane, fixed_t x, fixed_t y, fixed_t z);
bool P_PointBelowPlane(const plane_t *plane, fixed_t x, fixed_t y, fixed_t z);

struct v3fixed_t;
v3fixed_t P_LinePlaneIntersection(const plane_t *plane, const v3fixed_t &lineorg, const v3fixed_t &linedir);

bool P_CheckSightEdges(const AActor *t1, const AActor *t2, float radius_boost);
bool P_SpecialIsWeapon(AActor *special);

bool P_ChangeSector(sector_t *sector, int32_t crunch);

extern AActor *linetarget; // who got hit (or NULL)

fixed_t P_AimLineAttack(AActor *t1, angle_t angle, fixed_t distance);
fixed_t P_AutoAimLineAttack(AActor *actor, angle_t &angle, const angle_t spread, const int32_t tracers,
                            fixed_t distance);
void    P_LineAttack(AActor *t1, angle_t angle, fixed_t distance, fixed_t slope, int32_t damage);

// [RH] Position the chasecam
void           P_AimCamera(AActor *t1);
extern fixed_t CameraX, CameraY, CameraZ;

// [RH] Means of death
void P_RadiusAttack(AActor *spot, AActor *source, int32_t damage, int32_t distance, bool hurtSelf, int32_t mod);

void    P_DelSeclist(msecnode_t *);                            // phares 3/16/98
void    P_CreateSecNodeList(AActor *, fixed_t, fixed_t);       // phares 3/14/98
int32_t P_GetMoveFactor(const AActor *mo, int32_t *frictionp); // phares  3/6/98
int32_t P_GetFriction(const AActor *mo, int32_t *frictionfactor);
bool    Check_Sides(AActor *, int32_t, int32_t);               // phares

//
// P_SETUP
//
extern uint8_t *rejectmatrix; // for fast sight rejection
extern bool     rejectempty;
extern int32_t *blockmaplump; // offsets in blockmap are from here
extern int32_t *blockmap;
extern int32_t  bmapwidth;
extern int32_t  bmapheight;   // in mapblocks
extern fixed_t  bmaporgx;
extern fixed_t  bmaporgy;     // origin of block map
extern AActor **blocklinks;   // for thing chains

extern std::set<int16_t> movable_sectors;

//
// P_INTER
//
extern int32_t maxammo[NUMAMMO];
extern int32_t clipammo[NUMAMMO];

void P_GiveSpecial(player_t *player, AActor *special);
void P_TouchSpecialThing(AActor *special, AActor *toucher);

void P_DamageMobj(AActor *target, AActor *inflictor, AActor *source, int32_t damage, int32_t mod = 0,
                  int32_t flags = 0);

#define DMG_NO_ARMOR 1

// [RH] Means of death flags (based on Quake2's)
#define MOD_UNKNOWN       0
#define MOD_FIST          1
#define MOD_PISTOL        2
#define MOD_SHOTGUN       3
#define MOD_CHAINGUN      4
#define MOD_ROCKET        5
#define MOD_R_SPLASH      6
#define MOD_PLASMARIFLE   7
#define MOD_BFG_BOOM      8
#define MOD_BFG_SPLASH    9
#define MOD_CHAINSAW      10
#define MOD_SSHOTGUN      11
#define MOD_WATER         12
#define MOD_SLIME         13
#define MOD_LAVA          14
#define MOD_CRUSH         15
#define MOD_TELEFRAG      16
#define MOD_FALLING       17
#define MOD_SUICIDE       18
#define MOD_BARREL        19
#define MOD_EXIT          20
#define MOD_SPLASH        21
#define MOD_HIT           22
#define MOD_RAILGUN       23
#define MOD_FIREBALL      24 // Odamex-specific - monster fireball.
#define MOD_HITSCAN       25 // Odamex-specific - monster hitscan.
#define MOD_VILEFIRE      26 // Odamex-specific - vile fire.
#define NUMMODS           (MOD_VILEFIRE + 1)
#define MOD_FRIENDLY_FIRE 0x80000000

extern int32_t MeansOfDeath;

//
// PO_MAN
//
typedef enum
{
    PODOOR_NONE,
    PODOOR_SLIDE,
    PODOOR_SWING,

    NUMTYPES
} podoortype_t;

inline FArchive &operator<<(FArchive &arc, podoortype_t type)
{
    return arc << (uint8_t)type;
}
inline FArchive &operator>>(FArchive &arc, podoortype_t &out)
{
    uint8_t in;
    arc >> in;
    out = (podoortype_t)in;
    return arc;
}

class DPolyAction : public DThinker
{
    DECLARE_SERIAL(DPolyAction, DThinker)
  public:
    DPolyAction(int32_t polyNum);

  protected:
    DPolyAction();
    int32_t m_PolyObj;
    int32_t m_Speed;
    int32_t m_Dist;

    friend void ThrustMobj(AActor *actor, seg_t *seg, polyobj_t *po);
};

class DRotatePoly : public DPolyAction
{
    DECLARE_SERIAL(DRotatePoly, DPolyAction)
  public:
    DRotatePoly(int32_t polyNum);
    void RunThink();

  protected:
    friend bool EV_RotatePoly(line_t *line, int32_t polyNum, int32_t speed, int32_t byteAngle, int32_t direction,
                              bool overRide);

  private:
    DRotatePoly();
};

class DMovePoly : public DPolyAction
{
    DECLARE_SERIAL(DMovePoly, DPolyAction)
  public:
    DMovePoly(int32_t polyNum);
    void RunThink();

  protected:
    DMovePoly();
    int32_t m_Angle;
    fixed_t m_xSpeed; // for sliding walls
    fixed_t m_ySpeed;

    friend bool EV_MovePoly(line_t *line, int32_t polyNum, int32_t speed, angle_t angle, fixed_t dist, bool overRide);
};

class DPolyDoor : public DMovePoly
{
    DECLARE_SERIAL(DPolyDoor, DMovePoly)
  public:
    DPolyDoor(int32_t polyNum, podoortype_t type);
    void RunThink();

  protected:
    int32_t      m_Direction;
    int32_t      m_TotalDist;
    int32_t      m_Tics;
    int32_t      m_WaitTics;
    podoortype_t m_Type;
    bool         m_Close;

    friend bool EV_OpenPolyDoor(line_t *line, int32_t polyNum, int32_t speed, angle_t angle, int32_t delay,
                                int32_t distance, podoortype_t type);

  private:
    DPolyDoor();
};

// [RH] Data structure for P_SpawnMapThing() to keep track
//		of polyobject-related things.
typedef struct polyspawns_s
{
    struct polyspawns_s *next;
    fixed_t              x;
    fixed_t              y;
    int16_t              angle;
    int16_t              type;
} polyspawns_t;

enum
{
    PO_HEX_ANCHOR_TYPE = 3000,
    PO_HEX_SPAWN_TYPE,
    PO_HEX_SPAWNCRUSH_TYPE,

    // [RH] Thing numbers that don't conflict with Doom things
    PO_ANCHOR_TYPE = 9300,
    PO_SPAWN_TYPE,
    PO_SPAWNCRUSH_TYPE
};

#define PO_LINE_START    1       // polyobj line start special
#define PO_LINE_EXPLICIT 5

extern polyobj_t    *polyobjs;   // list of all poly-objects on the level
extern int32_t       po_NumPolyobjs;
extern polyspawns_t *polyspawns; // [RH] list of polyobject things to spawn

bool PO_MovePolyobj(int32_t num, int32_t x, int32_t y);
bool PO_RotatePolyobj(int32_t num, angle_t angle);
void PO_Init(void);
bool PO_Busy(int32_t polyobj);

bool P_CheckFov(AActor *t1, AActor *t2, angle_t fov);
bool P_IsFriendlyThing(AActor *actor, AActor *friendshiptest);
bool P_IsTeamMate(AActor *actor, AActor *player);
//
// P_SPEC
//
#include "p_spec.h"
