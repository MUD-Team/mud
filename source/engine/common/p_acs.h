// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 672d3042faa472fce4edea1254c6518a7607cb76 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	[RH] p_acs.h: ACS Script Stuff
//
//-----------------------------------------------------------------------------

#pragma once

#include "dobject.h"
#include "map_defs.h"

#define LOCAL_SIZE 20
#define STACK_SIZE 4096

struct ScriptPtr
{
    uint16_t Number;
    uint8_t  Type;
    uint8_t  ArgCount;
    uint32_t Address;
};

struct ScriptPtr1
{
    uint16_t Number;
    uint16_t Type;
    uint32_t Address;
    uint32_t ArgCount;
};

struct ScriptPtr2
{
    uint32_t Number; // Type is Number / 1000
    uint32_t Address;
    uint32_t ArgCount;
};

struct ScriptFunction
{
    uint8_t  ArgCount;
    uint8_t  LocalCount;
    uint8_t  HasReturnValue;
    uint8_t  Pad;
    uint32_t Address;
};

enum
{
    SCRIPT_Closed     = 0,
    SCRIPT_Open       = 1,
    SCRIPT_Respawn    = 2,
    SCRIPT_Death      = 3,
    SCRIPT_Enter      = 4,
    SCRIPT_Pickup     = 5,
    SCRIPT_T1Return   = 6,
    SCRIPT_T2Return   = 7,
    SCRIPT_Lightning  = 12,
    SCRIPT_Disconnect = 14,
};

enum ACSFormat
{
    ACS_Old,
    ACS_Enhanced,
    ACS_LittleEnhanced,
    ACS_Unknown
};

class FBehavior
{
  public:
    FBehavior(uint8_t *object, int32_t len);
    ~FBehavior();

    bool        IsGood();
    uint8_t    *FindChunk(uint32_t id) const;
    uint8_t    *NextChunk(uint8_t *chunk) const;
    int32_t    *FindScript(int32_t number) const;
    void        PrepLocale(uint32_t userpref, uint32_t userdef, uint32_t syspref, uint32_t sysdef);
    const char *LookupString(uint32_t index, uint32_t ofs = 0) const;
    const char *LocalizeString(uint32_t index) const;
    void     StartTypedScripts(uint16_t type, AActor *activator, int32_t arg0 = 0, int32_t arg1 = 0, int32_t arg2 = 0,
                               bool always = true) const;
    uint32_t PC2Ofs(int32_t *pc) const
    {
        return (uint8_t *)pc - Data;
    }
    int32_t *Ofs2PC(uint32_t ofs) const
    {
        return (int32_t *)(Data + ofs);
    }
    ACSFormat GetFormat() const
    {
        return Format;
    }
    ScriptFunction *GetFunction(int32_t funcnum) const;
    int32_t         GetArrayVal(int32_t arraynum, int32_t index) const;
    void            SetArrayVal(int32_t arraynum, int32_t index, int32_t value);

  private:
    struct ArrayInfo;

    ACSFormat Format;

    uint8_t   *Data;
    int32_t    DataSize;
    uint8_t   *Chunks;
    uint8_t   *Scripts;
    int32_t    NumScripts;
    uint8_t   *Functions;
    int32_t    NumFunctions;
    ArrayInfo *Arrays;
    int32_t    NumArrays;
    uint32_t   LanguageNeutral;
    uint32_t   Localized;

    static int32_t STACK_ARGS SortScripts(const void *a, const void *b);
    void                      AddLanguage(uint32_t lang);
    uint32_t                  FindLanguage(uint32_t lang, bool ignoreregion) const;
    uint32_t                 *CheckIfInList(uint32_t lang);
};

class DLevelScript : public DObject
{
    DECLARE_SERIAL(DLevelScript, DObject)
  public:
    // P-codes for ACS scripts
    enum
    {
        /*  0*/ PCD_NOP,
        PCD_TERMINATE,
        PCD_SUSPEND,
        PCD_PUSHNUMBER,
        PCD_LSPEC1,
        PCD_LSPEC2,
        PCD_LSPEC3,
        PCD_LSPEC4,
        PCD_LSPEC5,
        PCD_LSPEC1DIRECT,
        /* 10*/ PCD_LSPEC2DIRECT,
        PCD_LSPEC3DIRECT,
        PCD_LSPEC4DIRECT,
        PCD_LSPEC5DIRECT,
        PCD_ADD,
        PCD_SUBTRACT,
        PCD_MULTIPLY,
        PCD_DIVIDE,
        PCD_MODULUS,
        PCD_EQ,
        /* 20*/ PCD_NE,
        PCD_LT,
        PCD_GT,
        PCD_LE,
        PCD_GE,
        PCD_ASSIGNSCRIPTVAR,
        PCD_ASSIGNMAPVAR,
        PCD_ASSIGNWORLDVAR,
        PCD_PUSHSCRIPTVAR,
        PCD_PUSHMAPVAR,
        /* 30*/ PCD_PUSHWORLDVAR,
        PCD_ADDSCRIPTVAR,
        PCD_ADDMAPVAR,
        PCD_ADDWORLDVAR,
        PCD_SUBSCRIPTVAR,
        PCD_SUBMAPVAR,
        PCD_SUBWORLDVAR,
        PCD_MULSCRIPTVAR,
        PCD_MULMAPVAR,
        PCD_MULWORLDVAR,
        /* 40*/ PCD_DIVSCRIPTVAR,
        PCD_DIVMAPVAR,
        PCD_DIVWORLDVAR,
        PCD_MODSCRIPTVAR,
        PCD_MODMAPVAR,
        PCD_MODWORLDVAR,
        PCD_INCSCRIPTVAR,
        PCD_INCMAPVAR,
        PCD_INCWORLDVAR,
        PCD_DECSCRIPTVAR,
        /* 50*/ PCD_DECMAPVAR,
        PCD_DECWORLDVAR,
        PCD_GOTO,
        PCD_IFGOTO,
        PCD_DROP,
        PCD_DELAY,
        PCD_DELAYDIRECT,
        PCD_RANDOM,
        PCD_RANDOMDIRECT,
        PCD_THINGCOUNT,
        /* 60*/ PCD_THINGCOUNTDIRECT,
        PCD_TAGWAIT,
        PCD_TAGWAITDIRECT,
        PCD_POLYWAIT,
        PCD_POLYWAITDIRECT,
        PCD_CHANGEFLOOR,
        PCD_CHANGEFLOORDIRECT,
        PCD_CHANGECEILING,
        PCD_CHANGECEILINGDIRECT,
        PCD_RESTART,
        /* 70*/ PCD_ANDLOGICAL,
        PCD_ORLOGICAL,
        PCD_ANDBITWISE,
        PCD_ORBITWISE,
        PCD_EORBITWISE,
        PCD_NEGATELOGICAL,
        PCD_LSHIFT,
        PCD_RSHIFT,
        PCD_UNARYMINUS,
        PCD_IFNOTGOTO,
        /* 80*/ PCD_LINESIDE,
        PCD_SCRIPTWAIT,
        PCD_SCRIPTWAITDIRECT,
        PCD_CLEARLINESPECIAL,
        PCD_CASEGOTO,
        PCD_BEGINPRINT,
        PCD_ENDPRINT,
        PCD_PRINTSTRING,
        PCD_PRINTNUMBER,
        PCD_PRINTCHARACTER,
        /* 90*/ PCD_PLAYERCOUNT,
        PCD_GAMETYPE,
        PCD_GAMESKILL,
        PCD_TIMER,
        PCD_SECTORSOUND,
        PCD_AMBIENTSOUND,
        PCD_SOUNDSEQUENCE,
        PCD_SETLINETEXTURE,
        PCD_SETLINEBLOCKING,
        PCD_SETLINESPECIAL,
        /*100*/ PCD_THINGSOUND,
        PCD_ENDPRINTBOLD, // [RH] End of Hexen p-codes
        PCD_ACTIVATORSOUND,
        PCD_LOCALAMBIENTSOUND,
        PCD_SETLINEMONSTERBLOCKING,
        PCD_PLAYERBLUESKULL, // [BC] Start of new [Skull Tag] pcodes
        PCD_PLAYERREDSKULL,
        PCD_PLAYERYELLOWSKULL,
        PCD_PLAYERMASTERSKULL,
        PCD_PLAYERBLUECARD,
        /*110*/ PCD_PLAYERREDCARD,
        PCD_PLAYERYELLOWCARD,
        PCD_PLAYERMASTERCARD,
        PCD_PLAYERBLACKSKULL,
        PCD_PLAYERSILVERSKULL,
        PCD_PLAYERGOLDSKULL,
        PCD_PLAYERBLACKCARD,
        PCD_PLAYERSILVERCARD,
        PCD_PLAYERGOLDCARD,
        PCD_PLAYERTEAM1,
        /*120*/ PCD_PLAYERHEALTH,
        PCD_PLAYERARMORPOINTS,
        PCD_PLAYERFRAGS,
        PCD_PLAYEREXPERT,
        PCD_TEAM1COUNT,
        PCD_TEAM2COUNT,
        PCD_TEAM1SCORE,
        PCD_TEAM2SCORE,
        PCD_TEAM1FRAGPOINTS,
        PCD_LSPEC6,               // These are never used. They should probably
        /*130*/ PCD_LSPEC6DIRECT, // be given names like PCD_DUMMY.
        PCD_PRINTNAME,
        PCD_MUSICCHANGE,
        PCD_TEAM2FRAGPOINTS,
        PCD_CONSOLECOMMAND,
        PCD_SINGLEPLAYER, // [RH] End of Skull Tag p-codes
        PCD_FIXEDMUL,
        PCD_FIXEDDIV,
        PCD_SETGRAVITY,
        PCD_SETGRAVITYDIRECT,
        /*140*/ PCD_SETAIRCONTROL,
        PCD_SETAIRCONTROLDIRECT,
        PCD_CLEARINVENTORY,
        PCD_GIVEINVENTORY,
        PCD_GIVEINVENTORYDIRECT,
        PCD_TAKEINVENTORY,
        PCD_TAKEINVENTORYDIRECT,
        PCD_CHECKINVENTORY,
        PCD_CHECKINVENTORYDIRECT,
        PCD_SPAWN,
        /*150*/ PCD_SPAWNDIRECT,
        PCD_SPAWNSPOT,
        PCD_SPAWNSPOTDIRECT,
        PCD_SETMUSIC,
        PCD_SETMUSICDIRECT,
        PCD_LOCALSETMUSIC,
        PCD_LOCALSETMUSICDIRECT,
        PCD_PRINTFIXED,
        PCD_PRINTLOCALIZED,
        PCD_MOREHUDMESSAGE,
        /*160*/ PCD_OPTHUDMESSAGE,
        PCD_ENDHUDMESSAGE,
        PCD_ENDHUDMESSAGEBOLD,
        PCD_SETSTYLE,
        PCD_SETSTYLEDIRECT,
        PCD_SETFONT,
        PCD_SETFONTDIRECT,
        PCD_PUSHBYTE,
        PCD_LSPEC1DIRECTB,
        PCD_LSPEC2DIRECTB,
        /*170*/ PCD_LSPEC3DIRECTB,
        PCD_LSPEC4DIRECTB,
        PCD_LSPEC5DIRECTB,
        PCD_DELAYDIRECTB,
        PCD_RANDOMDIRECTB,
        PCD_PUSHBYTES,
        PCD_PUSH2BYTES,
        PCD_PUSH3BYTES,
        PCD_PUSH4BYTES,
        PCD_PUSH5BYTES,
        /*180*/ PCD_SETTHINGSPECIAL,
        PCD_ASSIGNGLOBALVAR,
        PCD_PUSHGLOBALVAR,
        PCD_ADDGLOBALVAR,
        PCD_SUBGLOBALVAR,
        PCD_MULGLOBALVAR,
        PCD_DIVGLOBALVAR,
        PCD_MODGLOBALVAR,
        PCD_INCGLOBALVAR,
        PCD_DECGLOBALVAR,
        /*190*/ PCD_FADETO,
        PCD_FADERANGE,
        PCD_CANCELFADE,
        PCD_PLAYMOVIE,
        PCD_SETFLOORTRIGGER,
        PCD_SETCEILINGTRIGGER,
        PCD_GETACTORX,
        PCD_GETACTORY,
        PCD_GETACTORZ,
        PCD_STARTTRANSLATION,
        /*200*/ PCD_TRANSLATIONRANGE1,
        PCD_TRANSLATIONRANGE2,
        PCD_ENDTRANSLATION,
        PCD_CALL,
        PCD_CALLDISCARD,
        PCD_RETURNVOID,
        PCD_RETURNVAL,
        PCD_PUSHMAPARRAY,
        PCD_ASSIGNMAPARRAY,
        PCD_ADDMAPARRAY,
        /*210*/ PCD_SUBMAPARRAY,
        PCD_MULMAPARRAY,
        PCD_DIVMAPARRAY,
        PCD_MODMAPARRAY,
        PCD_INCMAPARRAY,
        PCD_DECMAPARRAY,
        PCD_DUP,
        PCD_SWAP,
        PCD_WRITETOINI,
        PCD_GETFROMINI,
        /*220*/ PCD_SIN,
        PCD_COS,
        PCD_VECTORANGLE,

        // v THESE ARE UNUSED YET
        PCD_CHECKWEAPON,
        PCD_SETWEAPON,
        PCD_TAGSTRING,
        PCD_PUSHWORLDARRAY,
        PCD_ASSIGNWORLDARRAY,
        PCD_ADDWORLDARRAY,
        PCD_SUBWORLDARRAY,
        /*230*/ PCD_MULWORLDARRAY,
        PCD_DIVWORLDARRAY,
        PCD_MODWORLDARRAY,
        PCD_INCWORLDARRAY,
        PCD_DECWORLDARRAY,
        PCD_PUSHGLOBALARRAY,
        PCD_ASSIGNGLOBALARRAY,
        PCD_ADDGLOBALARRAY,
        PCD_SUBGLOBALARRAY,
        PCD_MULGLOBALARRAY,
        /*240*/ PCD_DIVGLOBALARRAY,
        PCD_MODGLOBALARRAY,
        PCD_INCGLOBALARRAY,
        PCD_DECGLOBALARRAY,
        PCD_SETMARINEWEAPON,
        PCD_SETACTORPROPERTY,
        PCD_GETACTORPROPERTY,
        // ^ THESE ARE UNUSED YET

        PCD_PLAYERNUMBER,
        PCD_ACTIVATORTID,
        PCD_GETCVAR               = 255,
        /*260*/ PCD_GETACTORANGLE = 260,
        PCD_GETLEVELINFO          = 265,

        PCODE_COMMAND_COUNT
    };

    static void ACS_SetLineTexture(int32_t *args, uint8_t argCount);
    static void ACS_ClearInventory(AActor *actor);
    static void ACS_Print(uint8_t pcd, AActor *actor, const char *print);
    static void ACS_ChangeMusic(uint8_t pcd, AActor *activator, int32_t *args, uint8_t argCount);
    static void ACS_StartSound(uint8_t pcd, AActor *activator, int32_t *args, uint8_t argCount);
    static void ACS_SetLineBlocking(int32_t *args, uint8_t argCount);
    static void ACS_SetLineMonsterBlocking(int32_t *args, uint8_t argCount);
    static void ACS_SetLineSpecial(int32_t *args, uint8_t argCount);
    static void ACS_SetThingSpecial(int32_t *args, uint8_t argCount);
    static void ACS_FadeRange(AActor *activator, int32_t *args, uint8_t argCount);
    static void ACS_CancelFade(AActor *activator);
    static void ACS_ChangeFlat(uint8_t pcd, int32_t *args, uint8_t argCount);
    static void ACS_SoundSequence(int32_t *args, uint8_t argCount);

    // Some constants used by ACS scripts
    enum
    {
        LINE_FRONT = 0,
        LINE_BACK  = 1
    };
    enum
    {
        SIDE_FRONT = 0,
        SIDE_BACK  = 1
    };
    enum
    {
        TEXTURE_TOP    = 0,
        TEXTURE_MIDDLE = 1,
        TEXTURE_BOTTOM = 2
    };
    enum
    {
        GAME_SINGLE_PLAYER      = 0,
        GAME_NET_COOPERATIVE    = 1,
        GAME_NET_DEATHMATCH     = 2,
        GAME_NET_TEAMDEATHMATCH = 3,
        GAME_NET_CTF            = 4
    };
    enum
    {
        CLASS_FIGHTER = 0,
        CLASS_CLERIC  = 1,
        CLASS_MAGE    = 2
    };
    enum
    {
        SKILL_VERY_EASY = 0,
        SKILL_EASY      = 1,
        SKILL_NORMAL    = 2,
        SKILL_HARD      = 3,
        SKILL_VERY_HARD = 4
    };
    enum
    {
        BLOCK_NOTHING    = 0,
        BLOCK_CREATURES  = 1,
        BLOCK_EVERYTHING = 2
    };

    enum
    {
        LEVELINFO_PAR_TIME,
        LEVELINFO_CLUSTERNUM,
        LEVELINFO_LEVELNUM,
        LEVELINFO_TOTAL_SECRETS,
        LEVELINFO_FOUND_SECRETS,
        LEVELINFO_TOTAL_ITEMS,
        LEVELINFO_FOUND_ITEMS,
        LEVELINFO_TOTAL_MONSTERS,
        LEVELINFO_KILLED_MONSTERS,
        LEVELINFO_SUCK_TIME
    };

    enum EScriptState
    {
        SCRIPT_Running,
        SCRIPT_Suspended,
        SCRIPT_Delayed,
        SCRIPT_TagWait,
        SCRIPT_PolyWait,
        SCRIPT_ScriptWaitPre,
        SCRIPT_ScriptWait,
        SCRIPT_PleaseRemove,
        SCRIPT_DivideBy0,
        SCRIPT_ModulusBy0,
    };

    DLevelScript(AActor *who, line_t *where, int32_t num, int32_t *code, int32_t lineSide, int32_t arg0, int32_t arg1,
                 int32_t arg2, int32_t always, bool delay);

    void RunScript();

    inline void SetState(EScriptState newstate)
    {
        state = newstate;
    }
    inline EScriptState GetState()
    {
        return state;
    }

    void *operator new(size_t size);
    void  operator delete(void *block);

  protected:
    DLevelScript *next, *prev;
    int32_t       script;
    int32_t       sp;
    int32_t       localvars[LOCAL_SIZE];
    int32_t      *pc;
    EScriptState  state;
    int32_t       statedata;
    AActor       *activator;
    line_t       *activationline;
    int32_t       lineSide;
    int32_t       stringstart;

    inline void PushToStack(int32_t val);

    void           Link();
    void           Unlink();
    void           PutLast();
    void           PutFirst();
    static int32_t Random(int32_t min, int32_t max);
    static int32_t ThingCount(int32_t type, int32_t tid);
    static void    ChangeFlat(int32_t tag, int32_t name, bool floorOrCeiling);
    static int32_t CountPlayers();
    static void    SetLineTexture(int32_t lineid, int32_t side, int32_t position, int32_t name);

    static int32_t DoSpawn(int32_t type, fixed_t x, fixed_t y, fixed_t z, int32_t tid, int32_t angle);
    static int32_t DoSpawnSpot(int32_t type, int32_t spot, int32_t tid, int32_t angle);

    static void SetLineBlocking(int32_t lineid, int32_t flags);
    static void SetLineMonsterBlocking(int32_t lineid, int32_t toggle);
    static void SetLineSpecial(int32_t lineid, int32_t special, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4,
                               int32_t arg5);
    static void ActivateLineSpecial(uint8_t special, line_t *line, AActor *activator, int32_t arg0, int32_t arg1,
                                    int32_t arg2, int32_t arg3, int32_t arg4);
    static void ChangeMusic(uint8_t pcd, AActor *activator, int32_t index, int32_t loop);
    static void StartSound(uint8_t pcd, AActor *activator, int32_t channel, int32_t index, int32_t volume,
                           int32_t attenuation);
    static void StartSectorSound(uint8_t pcd, sector_t *sector, int32_t channel, int32_t index, int32_t volume,
                                 int32_t attenuation);
    static void StartThingSound(uint8_t pcd, AActor *actor, int32_t channel, int32_t index, int32_t volume,
                                int32_t attenuation);
    static void SetThingSpecial(AActor *actor, int32_t special, int32_t arg1, int32_t arg2, int32_t arg3, int32_t arg4,
                                int32_t arg5);
    static void CancelFade(AActor *actor);
    static void StartSoundSequence(sector_t *sec, int32_t index);

    static void DoFadeTo(AActor *who, int32_t r, int32_t g, int32_t b, int32_t a, fixed_t time);
    static void DoFadeRange(AActor *who, int32_t r1, int32_t g1, int32_t b1, int32_t a1, int32_t r2, int32_t g2,
                            int32_t b2, int32_t a2, fixed_t time);

  private:
    DLevelScript();

    friend class DACSThinker;
};

inline FArchive &operator<<(FArchive &arc, DLevelScript::EScriptState state)
{
    return arc << (uint8_t)state;
}
inline FArchive &operator>>(FArchive &arc, DLevelScript::EScriptState &state)
{
    uint8_t in;
    arc >> in;
    state = (DLevelScript::EScriptState)in;
    return arc;
}

class DACSThinker : public DThinker
{
    DECLARE_SERIAL(DACSThinker, DThinker)
  public:
    DACSThinker();
    ~DACSThinker();

    void RunThink();

    DLevelScript       *RunningScripts[1000]; // Array of all synchronous scripts
    static DACSThinker *ActiveThinker;

    void DumpScriptStatus();

  private:
    DLevelScript *LastScript;
    DLevelScript *Scripts; // List of all running scripts

    friend class DLevelScript;
};

// The structure used to control scripts between maps
struct acsdefered_s
{
    struct acsdefered_s *next;

    enum EType
    {
        defexecute,
        defexealways,
        defsuspend,
        defterminate
    } type;
    int32_t script;
    int32_t arg0, arg1, arg2;
    int32_t playernum;
};
typedef struct acsdefered_s acsdefered_t;

FArchive &operator<<(FArchive &arc, acsdefered_s *defer);
FArchive &operator>>(FArchive &arc, acsdefered_s *&defer);
