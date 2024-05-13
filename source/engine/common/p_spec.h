// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 13c5694ca623b431308c054be7c427aa430710b9 $
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
// DESCRIPTION:  none
//	Implements special effects:
//	Texture animation, height or lighting changes
//	 according to adjacent sectors, respective
//	 utility functions, etc.
//
//-----------------------------------------------------------------------------

#pragma once

#include <list>

#include "d_player.h"
#include "dsectoreffect.h"
#include "r_state.h"
#include "res_texture.h"

typedef struct movingsector_s
{
    movingsector_s() : sector(NULL), moving_ceiling(false), moving_floor(false)
    {
    }

    sector_t *sector;
    bool      moving_ceiling;
    bool      moving_floor;
} movingsector_t;

enum motionspeed_e
{
    SpeedSlow,
    SpeedNormal,
    SpeedFast,
    SpeedTurbo,
};

enum doorkind_e
{
    OdCDoor,
    ODoor,
    CdODoor,
    CDoor,
};

enum floortarget_e
{
    FtoHnF,
    FtoLnF,
    FtoNnF,
    FtoLnC,
    FtoC,
    FbyST,
    Fby24,
    Fby32,
};

enum floorchange_e
{
    FNoChg,
    FChgZero,
    FChgTxt,
    FChgTyp,
};

enum ceilingtarget_e
{
    CtoHnC,
    CtoLnC,
    CtoNnC,
    CtoHnF,
    CtoF,
    CbyST,
    Cby24,
    Cby32,
};

enum ceilingchange_e
{
    CNoChg,
    CChgZero,
    CChgTxt,
    CChgTyp,
};

extern std::list<movingsector_t> movingsectors;
extern bool                      s_SpecialFromServer;

#define IgnoreSpecial !serverside && !s_SpecialFromServer
#define NO_TEXTURE    0;

#define CEILSPEED FRACUNIT

std::list<movingsector_t>::iterator P_FindMovingSector(sector_t *sector);
void                                P_AddMovingCeiling(sector_t *sector);
void                                P_AddMovingFloor(sector_t *sector);
void                                P_RemoveMovingCeiling(sector_t *sector);
void                                P_RemoveMovingFloor(sector_t *sector);
bool                                P_MovingCeilingCompleted(sector_t *sector);
bool                                P_MovingFloorCompleted(sector_t *sector);
bool                                P_HandleSpecialRepeat(line_t *line);
void                                P_ApplySectorDamage(player_t *player, int32_t damage, int32_t leak, int32_t mod = 0);
void                                P_ApplySectorDamageEndLevel(player_t *player);
void                                P_CollectSecretCommon(sector_t *sector, player_t *player);
int32_t                                 P_FindSectorFromTagOrLine(int32_t tag, const line_t *line, int32_t start);
int32_t                                 P_FindLineFromTag(int32_t tag, int32_t start);
bool                                P_FloorActive(const sector_t *sec);
bool                                P_LightingActive(const sector_t *sec);
bool                                P_CeilingActive(const sector_t *sec);
fixed_t                             P_ArgToSpeed(uint8_t arg);
bool                                P_ArgToCrushType(uint8_t arg);
void                                P_ResetSectorSpecial(sector_t *sector);
void                                P_CopySectorSpecial(sector_t *dest, sector_t *source);
uint8_t                                P_ArgToChange(uint8_t arg);
int32_t                                 P_ArgToCrush(uint8_t arg);
int32_t                                 P_FindSectorFromLineTag(const line_t *line, int32_t start);
int32_t                                 P_ArgToCrushMode(uint8_t arg, bool slowdown);
fixed_t                             P_ArgsToFixed(fixed_t arg_i, fixed_t arg_f);
bool                                P_CheckTag(line_t *line);
void                                P_TransferSectorFlags(uint32_t *, uint32_t);

// jff 2/23/98 identify the special classes that can share sectors

typedef enum
{
    floor_special,
    ceiling_special,
    lighting_special
} special_e;

enum crushmode_e
{
    crushDoom     = 0,
    crushHexen    = 1,
    crushSlowdown = 2,
};

enum lifttarget_e
{
    F2LnF,
    F2NnF,
    F2LnC,
    LnF2HnF,
};

enum LineActivationType
{
    LineCross,
    LineUse,
    LineShoot,
    LinePush,
    LineACS,
};

enum zdoom_lock_t
{
    zk_none         = 0,
    zk_red_card     = 1,
    zk_blue_card    = 2,
    zk_yellow_card  = 3,
    zk_red_skull    = 4,
    zk_blue_skull   = 5,
    zk_yellow_skull = 6,
    zk_any          = 100,
    zk_all          = 101,
    zk_red          = 129,
    zk_blue         = 130,
    zk_yellow       = 131,
    zk_redx         = 132, // not sure why these redundant ones exist
    zk_bluex        = 133,
    zk_yellowx      = 134,
    zk_each_color   = 229,
};

struct newspecial_s
{
    int16_t        special;
    uint32_t flags;
    int32_t          damageamount;
    int32_t          damageinterval;
    int32_t          damageleakrate;
};

#define FLOORSPEED FRACUNIT

bool P_CanUnlockZDoomDoor(player_t *player, zdoom_lock_t lock, bool remote);

// killough 3/7/98: Add generalized scroll effects

class DScroller : public DThinker
{
    DECLARE_SERIAL(DScroller, DThinker)
  public:
    enum EScrollType
    {
        sc_side,
        sc_floor,
        sc_ceiling,
        sc_carry,
        sc_carry_ceiling // killough 4/11/98: carry objects hanging on ceilings
    };

    DScroller(EScrollType type, fixed_t dx, fixed_t dy, int32_t control, int32_t affectee, int32_t accel);
    DScroller(fixed_t dx, fixed_t dy, const line_t *l, int32_t control, int32_t accel);

    void RunThink();

    bool AffectsWall(int32_t wallnum)
    {
        return m_Type == sc_side && m_Affectee == wallnum;
    }
    int32_t GetWallNum()
    {
        return m_Type == sc_side ? m_Affectee : -1;
    }
    void SetRate(fixed_t dx, fixed_t dy)
    {
        m_dx = dx;
        m_dy = dy;
    }
    bool IsType(EScrollType type) const
    {
        return type == m_Type;
    }
    int32_t GetAffectee() const
    {
        return m_Affectee;
    }
    int32_t GetAccel() const
    {
        return m_Accel;
    }
    int32_t GetControl() const
    {
        return m_Control;
    }

    EScrollType GetType() const
    {
        return m_Type;
    }
    fixed_t GetScrollX() const
    {
        return m_dx;
    }
    fixed_t GetScrollY() const
    {
        return m_dy;
    }

  protected:
    EScrollType m_Type;       // Type of scroll effect
    fixed_t     m_dx, m_dy;   // (dx,dy) scroll speeds
    int32_t         m_Affectee;   // Number of affected sidedef, sector, tag, or whatever
    int32_t         m_Control;    // Control sector (-1 if none) used to control scrolling
    fixed_t     m_LastHeight; // Last known height of control sector
    fixed_t     m_vdx, m_vdy; // Accumulated velocity if accelerative
    int32_t         m_Accel;      // Whether it's accelerative

  private:
    DScroller();
};

inline FArchive &operator<<(FArchive &arc, DScroller::EScrollType type)
{
    return arc << (uint8_t)type;
}
inline FArchive &operator>>(FArchive &arc, DScroller::EScrollType &out)
{
    uint8_t in;
    arc >> in;
    out = (DScroller::EScrollType)in;
    return arc;
}

// phares 3/20/98: added new model of Pushers for push/pull effects

class DPusher : public DThinker
{
    DECLARE_SERIAL(DPusher, DThinker)
  public:
    enum EPusher
    {
        p_push,
        p_pull,
        p_wind,
        p_current
    };

    DPusher();
    DPusher(EPusher type, line_t *l, int32_t magnitude, int32_t angle, AActor *source, int32_t affectee);
    int32_t CheckForSectorMatch(EPusher type, int32_t tag)
    {
        if (m_Type == type && sectors[m_Affectee].tag == tag)
            return m_Affectee;
        else
            return -1;
    }
    void ChangeValues(int32_t magnitude, int32_t angle)
    {
        angle_t ang = (angle << 24) >> ANGLETOFINESHIFT;
        m_Xmag      = (magnitude * finecosine[ang]) >> FRACBITS;
        m_Ymag      = (magnitude * finesine[ang]) >> FRACBITS;
        m_Magnitude = magnitude;
    }

    virtual void RunThink();

  protected:
    EPusher           m_Type;
    AActor::AActorPtr m_Source;    // Point source if point pusher
    int32_t               m_Xmag;      // X Strength
    int32_t               m_Ymag;      // Y Strength
    int32_t               m_Magnitude; // Vector strength for point pusher
    int32_t               m_Radius;    // Effective radius for point pusher
    int32_t               m_X;         // X of point source if point pusher
    int32_t               m_Y;         // Y of point source if point pusher
    int32_t               m_Affectee;  // Number of affected sector

    friend bool PIT_PushThing(AActor *thing);
};

inline FArchive &operator<<(FArchive &arc, DPusher::EPusher type)
{
    return arc << (uint8_t)type;
}
inline FArchive &operator>>(FArchive &arc, DPusher::EPusher &out)
{
    uint8_t in;
    arc >> in;
    out = (DPusher::EPusher)in;
    return arc;
}

bool P_CheckKeys(player_t *p, card_t lock, bool remote);

// Define values for map objects
#define MO_TELEPORTMAN 14

// [RH] If a deathmatch game, checks to see if noexit is enabled.
//		If so, it kills the player and returns false. Otherwise,
//		it returns true, and the player is allowed to live.
bool CheckIfExitIsGood(AActor *self);

// [Blair] ZDoom sector specials
void P_SpawnZDoomSectorSpecials(void);

// every tic
void P_UpdateSpecials(void);

// when needed
void P_CrossSpecialLine(line_t *line, int32_t side, AActor *thing, bool bossaction);
void P_ShootSpecialLine(AActor *thing, line_t *line);
bool P_UseSpecialLine(AActor *thing, line_t *line, int32_t side, bool bossaction);
bool P_PushSpecialLine(AActor *thing, line_t *line, int32_t side);

void P_PlayerInZDoomSector(player_t *player);
void P_ApplySectorFriction(int32_t tag, int32_t value, bool use_thinker);

//
// getSector()
// Will return a sector_t*
//	given the number of the current sector,
//	the line number and the side (0/1) that you want.
//
inline sector_t *getSector(int32_t currentSector, int32_t line, int32_t side)
{
    return sides[(sectors[currentSector].lines[line])->sidenum[side]].sector;
}

//
// getNextSector()
// Return sector_t * of sector next to current.
// NULL if not two-sided line
//
inline sector_t *getNextSector(line_t *line, sector_t *sec)
{
    if (!(line->flags & ML_TWOSIDED))
        return NULL;

    return line->frontsector == sec ? (line->backsector != sec ? line->backsector : NULL) : line->frontsector;
}

fixed_t P_FindLowestFloorSurrounding(sector_t *sec);
fixed_t P_FindHighestFloorSurrounding(sector_t *sec);

fixed_t P_FindNextHighestFloor(sector_t *sec);
fixed_t P_FindNextLowestFloor(sector_t *sec);

fixed_t P_FindLowestCeilingSurrounding(sector_t *sec);                     // jff 2/04/98
fixed_t P_FindHighestCeilingSurrounding(sector_t *sec);                    // jff 2/04/98

fixed_t P_FindNextLowestCeiling(sector_t *sec);                            // jff 2/04/98
fixed_t P_FindNextHighestCeiling(sector_t *sec);                           // jff 2/04/98

fixed_t P_FindShortestTextureAround(sector_t *sec);                        // jff 2/04/98
fixed_t P_FindShortestUpperAround(sector_t *sec);                          // jff 2/04/98

sector_t *P_FindModelFloorSector(fixed_t floordestheight, sector_t *sec);  // jff 02/04/98
sector_t *P_FindModelCeilingSector(fixed_t ceildestheight, sector_t *sec); // jff 02/04/98

int32_t P_FindSectorFromTag(int32_t tag, int32_t start);
int32_t P_FindLineFromID(int32_t id, int32_t start);

int32_t P_FindMinSurroundingLight(sector_t *sector, int32_t max);

sector_t *P_NextSpecialSector(sector_t *sec, int32_t type, sector_t *back2); // [RH]

//
// P_LIGHTS
//

class DLighting : public DSectorEffect
{
    DECLARE_SERIAL(DLighting, DSectorEffect);

  public:
    DLighting(sector_t *sector);

  protected:
    DLighting();
};

class DFireFlicker : public DLighting
{
    DECLARE_SERIAL(DFireFlicker, DLighting)
  public:
    DFireFlicker(sector_t *sector);
    DFireFlicker(sector_t *sector, int32_t upper, int32_t lower);
    void RunThink();
    int32_t  GetMaxLight() const
    {
        return m_MaxLight;
    }
    int32_t GetMinLight() const
    {
        return m_MinLight;
    }

  protected:
    int32_t m_Count;
    int32_t m_MaxLight;
    int32_t m_MinLight;

  private:
    DFireFlicker();
};

class DFlicker : public DLighting
{
    DECLARE_SERIAL(DFlicker, DLighting)
  public:
    DFlicker(sector_t *sector, int32_t upper, int32_t lower);
    void RunThink();
    int32_t  GetMaxLight() const
    {
        return m_MaxLight;
    }
    int32_t GetMinLight() const
    {
        return m_MinLight;
    }

  protected:
    int32_t m_Count;
    int32_t m_MaxLight;
    int32_t m_MinLight;

  private:
    DFlicker();
};

class DLightFlash : public DLighting
{
    DECLARE_SERIAL(DLightFlash, DLighting)
  public:
    DLightFlash(sector_t *sector);
    DLightFlash(sector_t *sector, int32_t min, int32_t max);
    void RunThink();
    int32_t  GetMaxLight() const
    {
        return m_MaxLight;
    }
    int32_t GetMinLight() const
    {
        return m_MinLight;
    }

  protected:
    int32_t m_Count;
    int32_t m_MaxLight;
    int32_t m_MinLight;
    int32_t m_MaxTime;
    int32_t m_MinTime;

  private:
    DLightFlash();
};

class DStrobe : public DLighting
{
    DECLARE_SERIAL(DStrobe, DLighting)
  public:
    DStrobe(sector_t *sector, int32_t utics, int32_t ltics, bool inSync);
    DStrobe(sector_t *sector, int32_t upper, int32_t lower, int32_t utics, int32_t ltics);
    void RunThink();
    int32_t  GetMaxLight() const
    {
        return m_MaxLight;
    }
    int32_t GetMinLight() const
    {
        return m_MinLight;
    }
    int32_t GetDarkTime() const
    {
        return m_DarkTime;
    }
    int32_t GetBrightTime() const
    {
        return m_BrightTime;
    }
    int32_t GetCount() const
    {
        return m_Count;
    }
    void SetCount(int32_t count)
    {
        m_Count = count;
    }

  protected:
    int32_t m_Count;
    int32_t m_MinLight;
    int32_t m_MaxLight;
    int32_t m_DarkTime;
    int32_t m_BrightTime;

  private:
    DStrobe();
};

class DGlow : public DLighting
{
    DECLARE_SERIAL(DGlow, DLighting)
  public:
    DGlow(sector_t *sector);
    void RunThink();

  protected:
    int32_t m_MinLight;
    int32_t m_MaxLight;
    int32_t m_Direction;

  private:
    DGlow();
};

// [RH] Glow from Light_Glow and Light_Fade specials
class DGlow2 : public DLighting
{
    DECLARE_SERIAL(DGlow2, DLighting)
  public:
    DGlow2(sector_t *sector, int32_t start, int32_t end, int32_t tics, bool oneshot);
    void RunThink();
    int32_t  GetStart() const
    {
        return m_Start;
    }
    int32_t GetEnd() const
    {
        return m_End;
    }
    int32_t GetMaxTics() const
    {
        return m_MaxTics;
    }
    bool GetOneShot() const
    {
        return m_OneShot;
    }

  protected:
    int32_t  m_Start;
    int32_t  m_End;
    int32_t  m_MaxTics;
    int32_t  m_Tics;
    bool m_OneShot;

  private:
    DGlow2();
};

// [RH] Phased light thinker
class DPhased : public DLighting
{
    DECLARE_SERIAL(DPhased, DLighting)
  public:
    DPhased(sector_t *sector);
    DPhased(sector_t *sector, int32_t baselevel, int32_t phase);
    void RunThink();
    uint8_t GetBaseLevel() const
    {
        return m_BaseLevel;
    }
    uint8_t GetPhase() const
    {
        return m_Phase;
    }

  protected:
    uint8_t m_BaseLevel;
    uint8_t m_Phase;

  private:
    DPhased();
    DPhased(sector_t *sector, int32_t baselevel);
    int32_t PhaseHelper(sector_t *sector, int32_t index, int32_t light, sector_t *prev);
};

#define GLOWSPEED    8
#define STROBEBRIGHT 5
#define FASTDARK     15
#define SLOWDARK     TICRATE

void EV_StartLightFlickering(int32_t tag, int32_t upper, int32_t lower);
void EV_StartLightStrobing(int32_t tag, int32_t upper, int32_t lower, int32_t utics, int32_t ltics);
void EV_StartLightStrobing(int32_t tag, int32_t utics, int32_t ltics);
void EV_TurnTagLightsOff(int32_t tag);
void EV_LightTurnOn(int32_t tag, int32_t bright);
void EV_LightChange(int32_t tag, int32_t value);
int32_t  EV_LightTurnOnPartway(int32_t tag, int32_t level);

void P_SpawnGlowingLight(sector_t *sector);

void EV_StartLightGlowing(int32_t tag, int32_t upper, int32_t lower, int32_t tics);
void EV_StartLightFading(int32_t tag, int32_t value, int32_t tics);

//
// P_SWITCH
//
typedef struct
{
    char  name1[9];
    char  name2[9];
    int16_t episode;

} switchlist_t;

#define STAIR_USE_SPECIALS 1
#define STAIR_SYNC         2

// 1 second, in ticks.
#define BUTTONTIME TICRATE

void P_ChangeSwitchTexture(line_t *line, int32_t useAgain, bool playsound);

void P_InitSwitchList();

void P_ProcessSwitchDef();

texhandle_t P_GetButtonTexture(line_t *line);
bool  P_GetButtonInfo(line_t *line, uint32_t &state, uint32_t &time);
bool  P_SetButtonInfo(line_t *line, uint32_t state, uint32_t time);

void P_UpdateButtons(client_t *cl);

//
// P_PLATS
//
class DPlat : public DMovingFloor
{
    DECLARE_SERIAL(DPlat, DMovingFloor);

  public:
    enum EPlatState
    {
        init = 0,
        up,
        down,
        waiting,
        in_stasis,
        midup,
        middown,
        finished,
        destroy,
        state_size
    };

    enum EPlatType
    {
        perpetualRaise,
        downWaitUpStay,
        raiseAndChange,
        raiseToNearestAndChange,
        blazeDWUS,
        genLift,    // jff added to support generalized Plat types
        genPerpetual,
        toggleUpDn, // jff 3/14/98 added to support instant toggle type
        platPerpetualRaise,
        platDownWaitUpStay,
        platDownWaitUpStayStone,
        platUpNearestWaitDownStay,
        platUpWaitDownStay,
        platDownByValue,
        platUpByValue,
        platUpByValueStay,
        platRaiseAndStay,
        platToggle,
        platDownToNearestFloor,
        platDownToLowestCeiling,
        platRaiseAndStayLockout
    };

    void RunThink();

    void SetState(uint8_t state, int32_t count)
    {
        m_Status = (EPlatState)state;
        m_Count  = count;
    }
    void GetState(uint8_t &state, int32_t &count)
    {
        state = (uint8_t)m_Status;
        count = m_Count;
    }

    DPlat(sector_t *sector);
    DPlat(sector_t *sector, DPlat::EPlatType type, fixed_t height, int32_t speed, int32_t delay, fixed_t lip);
    DPlat(sector_t *sector, int32_t target, int32_t delay, int32_t speed, int32_t trigger); // [Blair] Boom Generic Plat type
    DPlat      *Clone(sector_t *sec) const;
    friend void P_SetPlatDestroy(DPlat *plat);

    void PlayPlatSound();

    fixed_t    m_Speed;
    fixed_t    m_Low;
    fixed_t    m_High;
    int32_t        m_Wait;
    int32_t        m_Count;
    EPlatState m_Status;
    EPlatState m_OldStatus;
    bool       m_Crush;
    int32_t        m_Tag;
    EPlatType  m_Type;
    fixed_t    m_Height;
    fixed_t    m_Lip;

  protected:
    void Reactivate();
    void Stop();

  private:
    DPlat();

    friend bool EV_DoPlat(int32_t tag, line_t *line, EPlatType type, fixed_t height, int32_t speed, int32_t delay, fixed_t lip,
                          int32_t change);
    friend void EV_StopPlat(int32_t tag);
    friend void P_ActivateInStasis(int32_t tag);
    friend bool EV_DoGenLift(line_t *line);
};

inline FArchive &operator<<(FArchive &arc, DPlat::EPlatType type)
{
    return arc << (uint8_t)type;
}
inline FArchive &operator>>(FArchive &arc, DPlat::EPlatType &out)
{
    uint8_t in;
    arc >> in;
    out = (DPlat::EPlatType)in;
    return arc;
}
inline FArchive &operator<<(FArchive &arc, DPlat::EPlatState state)
{
    return arc << (uint8_t)state;
}
inline FArchive &operator>>(FArchive &arc, DPlat::EPlatState &out)
{
    uint8_t in;
    arc >> in;
    out = (DPlat::EPlatState)in;
    return arc;
}

//
// [RH]
// P_PILLAR
//

class DPillar : public DMover
{
    DECLARE_SERIAL(DPillar, DMover)
  public:
    enum EPillarState
    {
        init = 0,
        finished,
        destroy,
        state_size
    };

    enum EPillar
    {
        pillarBuild,
        pillarOpen
    };

    DPillar();
    DPillar(sector_t *sector, EPillar type, fixed_t speed, fixed_t height, fixed_t height2, int32_t crush, bool hexencrush);
    DPillar    *Clone(sector_t *sec) const;
    friend void P_SetPillarDestroy(DPillar *pillar);
    friend bool EV_DoZDoomPillar(DPillar::EPillar type, line_t *line, int32_t tag, fixed_t speed, fixed_t floordist,
                                 fixed_t ceilingdist, int32_t crush, bool hexencrush);

    void RunThink();
    void PlayPillarSound();

    EPillar m_Type;
    fixed_t m_FloorSpeed;
    fixed_t m_CeilingSpeed;
    fixed_t m_FloorTarget;
    fixed_t m_CeilingTarget;
    int32_t     m_Crush;
    bool    m_HexenCrush;

    EPillarState m_Status;
};

inline FArchive &operator<<(FArchive &arc, DPillar::EPillar type)
{
    return arc << (uint8_t)type;
}
inline FArchive &operator>>(FArchive &arc, DPillar::EPillar &out)
{
    uint8_t in;
    arc >> in;
    out = (DPillar::EPillar)in;
    return arc;
}
inline FArchive &operator<<(FArchive &arc, DPillar::EPillarState state)
{
    return arc << (uint8_t)state;
}
inline FArchive &operator>>(FArchive &arc, DPillar::EPillarState &out)
{
    uint8_t in;
    arc >> in;
    out = (DPillar::EPillarState)in;
    return arc;
}

bool EV_DoPillar(DPillar::EPillar type, int32_t tag, fixed_t speed, fixed_t height, fixed_t height2, bool crush);
void P_SpawnDoorCloseIn30(sector_t *sec);
void P_SpawnDoorRaiseIn5Mins(sector_t *sec);

//
// P_DOORS
//
class DDoor : public DMovingCeiling
{
    DECLARE_SERIAL(DDoor, DMovingCeiling)
  public:
    enum EVlDoor
    {
        doorClose,
        doorOpen,
        doorRaise,
        doorRaiseIn5Mins,
        doorCloseWaitOpen,
        close30ThenOpen,
        blazeRaise,
        blazeOpen,
        blazeClose,
        waitRaiseDoor,
        waitCloseDoor,

        // jff 02/05/98 add generalize door types
        genRaise,
        genBlazeRaise,
        genOpen,
        genBlazeOpen,
        genClose,
        genBlazeClose,
        genCdO,
        genBlazeCdO,
    };

    enum EDoorState
    {
        init = 0,
        opening,
        closing,
        waiting,
        reopening,
        finished,
        destroy,
        state_size
    };

    DDoor(sector_t *sector);
    // Boom Generic Door
    DDoor(sector_t *sec, line_t *ln, int32_t delay, int32_t time, int32_t trigger, int32_t speed);
    // Boom Generic Locked Door
    DDoor(sector_t *sec, line_t *ln, int32_t kind, int32_t trigger, int32_t speed);
    // Boom Compatible DDoor
    DDoor(sector_t *sec, line_t *ln, EVlDoor type, fixed_t speed, int32_t delay);
    // ZDoom Compatible DDoor
    DDoor(sector_t *sec, line_t *ln, EVlDoor type, fixed_t speed, int32_t topwait, uint8_t lighttag, int32_t topcountdown);
    DDoor *Clone(sector_t *sec) const;

    friend void P_SetDoorDestroy(DDoor *door);

    void RunThink();
    void PlayDoorSound();

    EVlDoor m_Type;
    fixed_t m_TopHeight;
    fixed_t m_Speed;

    // tics to wait at the top
    int32_t m_TopWait;
    // (keep in case a door going down is reset)
    // when it reaches 0, start going down
    int32_t m_TopCountdown;

    EDoorState m_Status;

    line_t *m_Line;

    int32_t m_LightTag; // ZDoom compat

  protected:
    friend bool EV_DoDoor(DDoor::EVlDoor type, line_t *line, AActor *thing, int32_t tag, int32_t speed, int32_t delay, card_t lock);
    friend bool EV_DoZDoomDoor(DDoor::EVlDoor type, line_t *line, AActor *mo, uint8_t tag, uint8_t speed_byte, int32_t topwait,
                               zdoom_lock_t lock, uint8_t lightTag, bool boomgen, int32_t topcountdown);
    friend void P_SpawnDoorCloseIn30(sector_t *sec);
    friend void P_SpawnDoorRaiseIn5Mins(sector_t *sec);

  private:
    DDoor();
};

inline FArchive &operator<<(FArchive &arc, DDoor::EVlDoor type)
{
    return arc << (uint8_t)type;
}
inline FArchive &operator>>(FArchive &arc, DDoor::EVlDoor &out)
{
    uint8_t in;
    arc >> in;
    out = (DDoor::EVlDoor)in;
    return arc;
}
inline FArchive &operator<<(FArchive &arc, DDoor::EDoorState state)
{
    return arc << (uint8_t)state;
}
inline FArchive &operator>>(FArchive &arc, DDoor::EDoorState &out)
{
    uint8_t in;
    arc >> in;
    out = (DDoor::EDoorState)in;
    return arc;
}

//
// P_CEILNG
//

// [RH] Changed these
class DCeiling : public DMovingCeiling
{
    DECLARE_SERIAL(DCeiling, DMovingCeiling)
  public:
    enum ECeilingState
    {
        init = 0,
        up,
        down,
        waiting,
        finished,
        destroy,
        state_size
    };

    enum ECeiling
    {
        lowerToFloor,
        raiseToHighest,
        lowerToLowest,
        lowerToMaxFloor,
        lowerAndCrush,
        crushAndRaise,
        fastCrushAndRaise,
        silentCrushAndRaise,

        ceilLowerByValue,
        ceilRaiseByValue,
        ceilMoveToValue,
        ceilLowerToHighestFloor,
        ceilLowerInstant,
        ceilRaiseInstant,
        ceilCrushAndRaise,
        ceilLowerAndCrush,
        ceilCrushRaiseAndStay,
        ceilRaiseToNearest,
        ceilLowerToLowest,
        ceilLowerToFloor,

        // The following are only used by Generic_Ceiling
        ceilRaiseToHighest,
        ceilLowerToHighest,
        ceilRaiseToLowest,
        ceilLowerToNearest,
        ceilRaiseToHighestFloor,
        ceilRaiseToFloor,
        ceilRaiseByTexture,
        ceilLowerByTexture,

        genCeiling,
        genCeilingChg0,
        genCeilingChgT,
        genCeilingChg,

        genCrusher,
        genSilentCrusher,
    };

    DCeiling(sector_t *sec);
    DCeiling(sector_t *sec, fixed_t speed1, fixed_t speed2, int32_t silent);
    DCeiling(sector_t *sec, line_t *line, int32_t speed, int32_t target, int32_t crush, int32_t change, int32_t direction, int32_t model);
    DCeiling(sector_t *sec, line_t *line, int32_t silent, int32_t speed);
    DCeiling   *Clone(sector_t *sec) const;
    friend void P_SetCeilingDestroy(DCeiling *ceiling);

    void RunThink();
    void PlayCeilingSound();

    ECeiling    m_Type;
    crushmode_e m_CrushMode;
    fixed_t     m_BottomHeight;
    fixed_t     m_TopHeight;
    fixed_t     m_Speed;
    fixed_t     m_Speed1;    // [RH] dnspeed of crushers
    fixed_t     m_Speed2;    // [RH] upspeed of crushers
    int32_t         m_Crush;
    int32_t         m_Silent;
    int32_t         m_Direction; // 1 = up, 0 = waiting, -1 = down

    // [RH] Need these for BOOM-ish transferring ceilings
    texhandle_t   m_Texture;
    int16_t m_NewSpecial;
    uint32_t m_NewFlags;
    int16_t m_NewDamageRate;
    uint8_t  m_NewLeakRate;
    uint8_t  m_NewDmgInterval;

    // ID
    int32_t m_Tag;
    int32_t m_OldDirection;

    ECeilingState m_Status;

  protected:
  private:
    DCeiling();

    friend bool EV_DoCeiling(DCeiling::ECeiling type, line_t *line, int32_t tag, fixed_t speed, fixed_t speed2,
                             fixed_t height, bool crush, int32_t silent, int32_t change);
    friend bool EV_CeilingCrushStop(int32_t tag);
    friend void P_ActivateInStasisCeiling(int32_t tag);
    friend bool EV_ZDoomCeilingCrushStop(int32_t tag, bool remove);
};

inline FArchive &operator<<(FArchive &arc, DCeiling::ECeiling type)
{
    return arc << (uint8_t)type;
}
inline FArchive &operator>>(FArchive &arc, DCeiling::ECeiling &type)
{
    uint8_t in;
    arc >> in;
    type = (DCeiling::ECeiling)in;
    return arc;
}
inline FArchive &operator<<(FArchive &arc, DCeiling::ECeilingState state)
{
    return arc << (uint8_t)state;
}
inline FArchive &operator>>(FArchive &arc, DCeiling::ECeilingState &out)
{
    uint8_t in;
    arc >> in;
    out = (DCeiling::ECeilingState)in;
    return arc;
}

//
// P_FLOOR
//

class DFloor : public DMovingFloor
{
    DECLARE_SERIAL(DFloor, DMovingFloor)
  public:
    enum EFloorState
    {
        init = 0,
        up,
        down,
        waiting,
        finished,
        destroy,
        state_size
    };

    enum EFloor
    {
        floorLowerToLowest,
        floorLowerToNearest,
        floorLowerToHighest,
        floorLowerByValue,
        floorRaiseByValue,
        floorRaiseToHighest,
        floorRaiseToNearest,
        floorRaiseAndCrush,
        floorRaiseAndCrushDoom,
        floorCrushStop,
        floorLowerInstant,
        floorRaiseInstant,
        floorMoveToValue,
        floorRaiseToLowestCeiling,
        floorRaiseByTexture,

        floorLowerAndChange,
        floorRaiseAndChange,

        floorRaiseToLowest,
        floorRaiseToCeiling,
        floorLowerToLowestCeiling,
        floorLowerByTexture,
        floorLowerToCeiling,

        donutRaise,

        genBuildStair,
        buildStair,
        waitStair,
        resetStair,

        // Not to be used as parameters to EV_DoFloor()
        genFloor,
        genFloorChg0,
        genFloorChgT,
        genFloorChg
    };

    // [RH] Changed to use Hexen-ish specials
    enum EStair
    {
        buildUp,
        buildDown
    };

    DFloor(sector_t *sec);
    DFloor(sector_t *sec, DFloor::EFloor floortype, line_t *line, fixed_t speed, fixed_t height, bool crush,
           int32_t change);
    DFloor(sector_t *sec, line_t *line, int32_t speed, int32_t target, int32_t crush, int32_t change, int32_t direction, int32_t model);
    DFloor(sector_t *sec, DFloor::EFloor floortype, line_t *line, fixed_t speed, fixed_t height, int32_t crush, int32_t change,
           bool hexencrush, bool hereticlower);
    DFloor     *Clone(sector_t *sec) const;
    friend void P_SetFloorDestroy(DFloor *floor);
    friend bool EV_DoGenFloor(line_t *line);
    friend bool EV_DoGenStairs(line_t *line);

    void RunThink();
    void PlayFloorSound();

    EFloor      m_Type;
    EFloorState m_Status;
    int32_t         m_Crush;
    bool        m_HexenCrush;
    int32_t         m_Direction;
    int16_t       m_NewSpecial;
    uint32_t       m_NewFlags;
    int16_t       m_NewDamageRate;
    uint8_t        m_NewLeakRate;
    uint8_t        m_NewDmgInterval;
    texhandle_t       m_Texture;
    fixed_t     m_FloorDestHeight;
    fixed_t     m_Speed;

    // [RH] New parameters used to reset and delay stairs
    int32_t m_ResetCount;
    int32_t m_OrgHeight;
    int32_t m_Delay;
    int32_t m_PauseTime;
    int32_t m_StepTime;
    int32_t m_PerStepTime;

    fixed_t m_Height;
    line_t *m_Line;
    int32_t     m_Change;

  protected:
    friend bool EV_BuildStairs(int32_t tag, DFloor::EStair type, line_t *line, fixed_t stairsize, fixed_t speed, int32_t delay,
                               int32_t reset, int32_t igntxt, int32_t usespecials);
    friend bool EV_DoFloor(DFloor::EFloor floortype, line_t *line, int32_t tag, fixed_t speed, fixed_t height, bool crush,
                           int32_t change);
    friend int32_t  EV_DoDonut(line_t *line);
    friend bool EV_DoZDoomDonut(int32_t tag, line_t *line, fixed_t pillarspeed, fixed_t slimespeed);
    friend int32_t  P_SpawnDonut(int32_t tag, line_t *line, fixed_t pillarspeed, fixed_t slimespeed);
    friend bool EV_DoZDoomFloor(DFloor::EFloor floortype, line_t *line, int32_t tag, fixed_t speed, fixed_t height,
                                int32_t crush, int32_t change, bool hexencrush, bool hereticlower);

  private:
    DFloor();
};

inline FArchive &operator<<(FArchive &arc, DFloor::EFloor type)
{
    return arc << (uint8_t)type;
}
inline FArchive &operator>>(FArchive &arc, DFloor::EFloor &type)
{
    uint8_t in;
    arc >> in;
    type = (DFloor::EFloor)in;
    return arc;
}
inline FArchive &operator<<(FArchive &arc, DFloor::EFloorState state)
{
    return arc << (uint8_t)state;
}
inline FArchive &operator>>(FArchive &arc, DFloor::EFloorState &out)
{
    uint8_t in;
    arc >> in;
    out = (DFloor::EFloorState)in;
    return arc;
}

class DElevator : public DMover
{
    DECLARE_SERIAL(DElevator, DMover)
  public:
    enum EElevatorState
    {
        init = 0,
        finished,
        destroy,
        state_size
    };

    enum EElevator
    {
        elevateUp,
        elevateDown,
        elevateCurrent,
        // [RH] For FloorAndCeiling_Raise/Lower
        elevateRaise,
        elevateLower
    };

    DElevator(sector_t *sec);
    DElevator  *Clone(sector_t *sec) const;
    friend void P_SetElevatorDestroy(DElevator *elevator);

    void RunThink();
    void PlayElevatorSound();

    EElevator m_Type;
    int32_t       m_Direction;
    fixed_t   m_FloorDestHeight;
    fixed_t   m_CeilingDestHeight;
    fixed_t   m_Speed;

    EElevatorState m_Status;

  protected:
    friend bool EV_DoElevator(line_t *line, DElevator::EElevator type, fixed_t speed, fixed_t height, int32_t tag);
    friend bool EV_DoZDoomElevator(line_t *line, DElevator::EElevator type, fixed_t speed, fixed_t height, int32_t tag);

  private:
    DElevator();
};

inline FArchive &operator<<(FArchive &arc, DElevator::EElevator type)
{
    return arc << (uint8_t)type;
}
inline FArchive &operator>>(FArchive &arc, DElevator::EElevator &out)
{
    uint8_t in;
    arc >> in;
    out = (DElevator::EElevator)in;
    return arc;
}
inline FArchive &operator<<(FArchive &arc, DElevator::EElevatorState state)
{
    return arc << (uint8_t)state;
}
inline FArchive &operator>>(FArchive &arc, DElevator::EElevatorState &out)
{
    uint8_t in;
    arc >> in;
    out = (DElevator::EElevatorState)in;
    return arc;
}

// Waggle
/*
class DWaggle : public DMover
{
    DECLARE_SERIAL(DWaggle, DMover)
  public:
    enum EWaggleState
    {
        init = 0,
        expand,
        reduce,
        stable,
        finished,
        destroy,
        state_size
    };
    DWaggle(sector_t* sec);
    DWaggle(sector_t* sector, int32_t height, int32_t speed, int32_t offset, int32_t timer,
                     bool ceiling);
    DWaggle* Clone(sector_t* sec) const;
    friend void P_SetWaggleDestroy(DWaggle* waggle);

    void RunThink();

    fixed_t m_OriginalHeight;
    fixed_t m_Accumulator;
    fixed_t m_AccDelta;
    fixed_t m_TargetScale;
    fixed_t m_Scale;
    fixed_t m_ScaleDelta;
    fixed_t m_StartTic; // [Blair] Client will predict a created (or serialized) waggle based on the start tic.
    int32_t m_Ticker;
    int32_t m_State;
    bool m_Ceiling;
    DWaggle();

  protected:
    friend bool EV_StartPlaneWaggle(int32_t tag, line_t* line, int32_t height, int32_t speed,
                                    int32_t offset, int32_t timer, bool ceiling);
};
*/
// jff 3/15/98 pure texture/type change for better generalized support
enum EChange
{
    trigChangeOnly,
    numChangeOnly
};

bool EV_DoChange(line_t *line, EChange changetype, int32_t tag);

//
// P_TELEPT
//
bool EV_Teleport(int32_t tid, int32_t tag, int32_t arg0, int32_t side, AActor *thing, int32_t nostop);
bool EV_LineTeleport(line_t *line, int32_t side, AActor *thing);
bool EV_SilentTeleport(int32_t tid, int32_t useangle, int32_t tag, int32_t keepheight, line_t *line, int32_t side, AActor *thing);
bool EV_SilentLineTeleport(line_t *line, int32_t side, AActor *thing, int32_t id, bool reverse);

//
// [RH] ACS (see also p_acs.h)
//

bool P_StartScript(AActor *who, line_t *where, int32_t script, const char *map, int32_t lineSide, int32_t arg0, int32_t arg1, int32_t arg2,
                   int32_t always);
void P_SuspendScript(int32_t script, const char *map);
void P_TerminateScript(int32_t script, const char *map);
void P_StartOpenScripts(void);
void P_DoDeferedScripts(void);

//
// [RH] p_quake.c
//
bool P_StartQuake(int32_t tid, int32_t intensity, int32_t duration, int32_t damrad, int32_t tremrad);

// [AM] Trigger actor specials.
bool A_TriggerAction(AActor *mo, AActor *triggerer, int32_t activationType);
