// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: a7441bb342fcbe5e84b7fa70be0e7e12298df2a6 $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	The not so system specific sound interface.
//
//-----------------------------------------------------------------------------

#pragma once

#include <map>
#include <vector>

#include "doomtype.h"
#include "m_fixed.h"

// Formerly in d_dehacked.cpp; will likely ditch this as things progress - Dasho
constexpr const char *SoundMap[] = {
    NULL, "weapons/pistol", "weapons/shotgf", "weapons/shotgr", "weapons/sshotf", "weapons/sshoto", "weapons/sshotc",
    "weapons/sshotl", "weapons/plasmaf", "weapons/bfgf", "weapons/sawup", "weapons/sawidle", "weapons/sawfull",
    "weapons/sawhit", "weapons/rocklf", "weapons/bfgx", "imp/attack", "imp/shotx", "plats/pt1_strt", "plats/pt1_stop",
    "doors/dr1_open", "doors/dr1_clos", "plats/pt1_mid", "switches/normbutn", "switches/exitbutn", "*pain100_1",
    "demon/pain", "grunt/pain", "vile/pain", "fatso/pain", "pain/pain", "misc/gibbed", "misc/i_pkup", "misc/w_pkup",
    "*land1", "misc/teleport", "grunt/sight1", "grunt/sight2", "grunt/sight3", "imp/sight1", "imp/sight2",
    "demon/sight", "caco/sight", "baron/sight", "cyber/sight", "spider/sight", "baby/sight", "knight/sight",
    "vile/sight", "fatso/sight", "pain/sight", "skull/melee", "demon/melee", "skeleton/melee", "vile/start",
    "imp/melee", "skeleton/swing", "*death1", "*xdeath1", "grunt/death1", "grunt/death2", "grunt/death3", "imp/death1",
    "imp/death2", "demon/death", "caco/death", "misc/unused", "baron/death", "cyber/death", "spider/death",
    "baby/death", "vile/death", "knight/death", "pain/death", "skeleton/death", "grunt/active", "imp/active",
    "demon/active", "baby/active", "baby/walk", "vile/active", "*grunt1", "world/barrelx", "*fist", "cyber/hoof",
    "spider/walk", "weapons/chngun", "misc/chat2", "doors/dr2_open", "doors/dr2_clos", "misc/spawn", "vile/firecrkl",
    "vile/firestrt", "misc/p_pkup", "brain/spit", "brain/cube", "brain/sight", "brain/pain", "brain/death",
    "fatso/attack", "gatso/death", "wolfss/sight", "wolfss/death", "keen/pain", "keen/death", "skeleton/active",
    "skeleton/sight", "skeleton/attack", "misc/chat",

    // MBF SOUNDS
    "dog/sight", "dog/attack", "dog/active", "dog/death", "dog/pain",

    // Padding -- DEHEXTRA's new sound range
    // starts at sound ID 500, so we need
    // to insert a bunch of blanks in between.
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "",

    // Crispy/Retro (DEHEXTRA)
    "dehextra/sound000", "dehextra/sound001", "dehextra/sound002", "dehextra/sound003", "dehextra/sound004",
    "dehextra/sound005", "dehextra/sound006", "dehextra/sound007", "dehextra/sound008", "dehextra/sound009",
    "dehextra/sound010", "dehextra/sound011", "dehextra/sound012", "dehextra/sound013", "dehextra/sound014",
    "dehextra/sound015", "dehextra/sound016", "dehextra/sound017", "dehextra/sound018", "dehextra/sound019",
    "dehextra/sound020", "dehextra/sound021", "dehextra/sound022", "dehextra/sound023", "dehextra/sound024",
    "dehextra/sound025", "dehextra/sound026", "dehextra/sound027", "dehextra/sound028", "dehextra/sound029",
    "dehextra/sound030", "dehextra/sound031", "dehextra/sound032", "dehextra/sound033", "dehextra/sound034",
    "dehextra/sound035", "dehextra/sound036", "dehextra/sound037", "dehextra/sound038", "dehextra/sound039",
    "dehextra/sound040", "dehextra/sound041", "dehextra/sound042", "dehextra/sound043", "dehextra/sound044",
    "dehextra/sound045", "dehextra/sound046", "dehextra/sound047", "dehextra/sound048", "dehextra/sound049",
    "dehextra/sound050", "dehextra/sound051", "dehextra/sound052", "dehextra/sound053", "dehextra/sound054",
    "dehextra/sound055", "dehextra/sound056", "dehextra/sound057", "dehextra/sound058", "dehextra/sound059",
    "dehextra/sound060", "dehextra/sound061", "dehextra/sound062", "dehextra/sound063", "dehextra/sound064",
    "dehextra/sound065", "dehextra/sound066", "dehextra/sound067", "dehextra/sound068", "dehextra/sound069",
    "dehextra/sound070", "dehextra/sound071", "dehextra/sound072", "dehextra/sound073", "dehextra/sound074",
    "dehextra/sound075", "dehextra/sound076", "dehextra/sound077", "dehextra/sound078", "dehextra/sound079",
    "dehextra/sound080", "dehextra/sound081", "dehextra/sound082", "dehextra/sound083", "dehextra/sound084",
    "dehextra/sound085", "dehextra/sound086", "dehextra/sound087", "dehextra/sound088", "dehextra/sound089",
    "dehextra/sound090", "dehextra/sound091", "dehextra/sound092", "dehextra/sound093", "dehextra/sound094",
    "dehextra/sound095", "dehextra/sound096", "dehextra/sound097", "dehextra/sound098", "dehextra/sound099",
    "dehextra/sound100", "dehextra/sound101", "dehextra/sound102", "dehextra/sound103", "dehextra/sound104",
    "dehextra/sound105", "dehextra/sound106", "dehextra/sound107", "dehextra/sound108", "dehextra/sound109",
    "dehextra/sound110", "dehextra/sound111", "dehextra/sound112", "dehextra/sound113", "dehextra/sound114",
    "dehextra/sound115", "dehextra/sound116", "dehextra/sound117", "dehextra/sound118", "dehextra/sound119",
    "dehextra/sound120", "dehextra/sound121", "dehextra/sound122", "dehextra/sound123", "dehextra/sound124",
    "dehextra/sound125", "dehextra/sound126", "dehextra/sound127", "dehextra/sound128", "dehextra/sound129",
    "dehextra/sound130", "dehextra/sound131", "dehextra/sound132", "dehextra/sound133", "dehextra/sound134",
    "dehextra/sound135", "dehextra/sound136", "dehextra/sound137", "dehextra/sound138", "dehextra/sound139",
    "dehextra/sound140", "dehextra/sound141", "dehextra/sound142", "dehextra/sound143", "dehextra/sound144",
    "dehextra/sound145", "dehextra/sound146", "dehextra/sound147", "dehextra/sound148", "dehextra/sound149",
    "dehextra/sound150", "dehextra/sound151", "dehextra/sound152", "dehextra/sound153", "dehextra/sound154",
    "dehextra/sound155", "dehextra/sound156", "dehextra/sound157", "dehextra/sound158", "dehextra/sound159",
    "dehextra/sound160", "dehextra/sound161", "dehextra/sound162", "dehextra/sound163", "dehextra/sound164",
    "dehextra/sound165", "dehextra/sound166", "dehextra/sound167", "dehextra/sound168", "dehextra/sound169",
    "dehextra/sound170", "dehextra/sound171", "dehextra/sound172", "dehextra/sound173", "dehextra/sound174",
    "dehextra/sound175", "dehextra/sound176", "dehextra/sound177", "dehextra/sound178", "dehextra/sound179",
    "dehextra/sound180", "dehextra/sound181", "dehextra/sound182", "dehextra/sound183", "dehextra/sound184",
    "dehextra/sound185", "dehextra/sound186", "dehextra/sound187", "dehextra/sound188", "dehextra/sound189",
    "dehextra/sound190", "dehextra/sound191", "dehextra/sound192", "dehextra/sound193", "dehextra/sound194",
    "dehextra/sound195", "dehextra/sound196", "dehextra/sound197", "dehextra/sound198", "dehextra/sound199",

    // ZDOOM-Specific sounds
    "misc/teamchat"};

#define MAX_SNDNAME 63

class AActor;
class player_s;
typedef player_s player_t;

//
// SoundFX struct.
//
typedef struct sfxinfo_struct sfxinfo_t;

struct sfxinfo_struct
{
    char     name[MAX_SNDNAME + 1]; // [RH] Sound name defined in SNDINFO
    unsigned normal;                // Normal sample handle
    unsigned looping;               // Looping sample handle
    void    *data;

    int link;
    enum
    {
        NO_LINK = 0xffffffff
    };

    int          lumpnum;     // lump number of sfx
    unsigned int ms;          // [RH] length of sfx in milliseconds
    unsigned int next, index; // [RH] For hashing
    unsigned int frequency;   // [RH] Preferred playback rate
    unsigned int length;      // [RH] Length of the sound in bytes
    bool         israndom;    // [DE] Whether or not this is an alias for a set of random sounds
};

// the complete set of sound effects
extern std::vector<sfxinfo_t> S_sfx;

// map of every sound id for sounds that have randomized variants
extern std::map<int, std::vector<int>> S_rnd;

// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//	allocates channel buffer, sets S_sfx lookup.
//
void S_Init(float sfxVolume, float musicVolume);
void S_Deinit();

// Per level startup code.
// Kills playing sounds at start of level,
//	determines music if any, changes music.
//
void S_Stop();
void S_Start();

// Start sound for thing at <ent>
void S_Sound(int channel, const char *name, float volume, int attenuation);
void S_Sound(AActor *ent, int channel, const char *name, float volume, int attenuation);
void S_Sound(fixed_t *pt, int channel, const char *name, float volume, int attenuation);
void S_Sound(fixed_t x, fixed_t y, int channel, const char *name, float volume, int attenuation);
void S_PlatSound(fixed_t *pt, int channel, const char *name, float volume,
                 int attenuation); // [Russell] - Hack to stop multiple plat stop sounds
void S_LoopedSound(AActor *ent, int channel, const char *name, float volume, int attenuation);
void S_LoopedSound(fixed_t *pt, int channel, const char *name, float volume, int attenuation);
void S_SoundID(int channel, int sfxid, float volume, int attenuation);
void S_SoundID(fixed_t x, fixed_t y, int channel, int sound_id, float volume, int attenuation);
void S_SoundID(AActor *ent, int channel, int sfxid, float volume, int attenuation);
void S_SoundID(fixed_t *pt, int channel, int sfxid, float volume, int attenuation);
void S_LoopedSoundID(AActor *ent, int channel, int sfxid, float volume, int attenuation);
void S_LoopedSoundID(fixed_t *pt, int channel, int sfxid, float volume, int attenuation);

// sound channels
// channel 0 never willingly overrides
// other channels (1-8) always override a playing sound on that channel
#define CHAN_AUTO      0
#define CHAN_WEAPON    1
#define CHAN_VOICE     2
#define CHAN_ITEM      3
#define CHAN_BODY      4
#define CHAN_ANNOUNCER 5
#define CHAN_GAMEINFO  6
#define CHAN_INTERFACE 7

// modifier flags
// #define CHAN_NO_PHS_ADD		8	// send to all clients, not just ones in PHS (ATTN 0 will
// also do this) #define CHAN_RELIABLE			16	// send by reliable message, not
// datagram

// sound attenuation values
#define ATTN_NONE   0 // full volume the entire level
#define ATTN_NORM   1
#define ATTN_IDLE   2
#define ATTN_STATIC 3 // diminish very rapidly with distance

// Stops a sound emanating from one of an entity's channels
void S_StopSound(AActor *ent, int channel);
void S_StopSound(fixed_t *pt, int channel);
void S_StopSound(fixed_t *pt);

// Stop sound for all channels
void S_StopAllChannels();

// Is the sound playing on one of the entity's channels?
bool S_GetSoundPlayingInfo(AActor *ent, int sound_id);
bool S_GetSoundPlayingInfo(fixed_t *pt, int sound_id);

// Moves all sounds from one mobj to another
void S_RelinkSound(AActor *from, AActor *to);

// Start music using <music_name>
void S_StartMusic(const char *music_name);

// Start music using <music_name>, and set whether looping
void S_ChangeMusic(std::string music_name, int looping);

// Stops the music fer sure.
void S_StopMusic();

// Stop and resume music, during game PAUSE.
void S_PauseSound();
void S_ResumeSound();

//
// Updates music & sounds
//
void S_UpdateSounds(void *listener);
void S_UpdateMusic();

void S_SetMusicVolume(float volume);
void S_SetSfxVolume(float volume);

// [RH] Activates an ambient sound. Called when the thing is added to the map.
//		(0-biased)
void S_ActivateAmbient(AActor *mobj, int ambient);

// [RH] S_sfx "maintenance" routines
void S_ParseSndInfo();

void S_HashSounds();
int  S_FindSound(const char *logicalname);
int  S_FindSoundByLump(int lump);
int  S_AddSound(const char *logicalname, const char *lumpname); // Add sound by lumpname
int  S_AddSoundLump(char *logicalname, int lump);               // Add sound by lump index
void S_AddRandomSound(int owner, std::vector<int> &list);
void S_ClearSoundLumps();

void UV_SoundAvoidPlayer(AActor *mo, byte channel, const char *name, byte attenuation);

// [RH] Prints sound debug info to the screen.
//		Modelled after Hexen's noise cheat.
void S_NoiseDebug();

// The following functions work seamlessly on local clients and networked games.

#if SERVER_APP
#include "sv_main.h"
#endif

static void S_NetSound(AActor *mo, byte channel, const char *name, const byte attenuation)
{
#if SERVER_APP
    SV_Sound(mo, channel, name, attenuation);
#else
    S_Sound(mo, channel, name, 1, attenuation);
#endif
}

static void S_PlayerSound(player_t *pl, AActor *mo, const byte channel, const char *name, const byte attenuation)
{
#if SERVER_APP
    SV_Sound(*pl, mo, channel, name, attenuation);
#else
    S_Sound(mo, channel, name, 1, attenuation);
#endif
}
