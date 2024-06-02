// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 0c70e151a0a895c0961e9df90b31e95a9cb6ec76 $
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	none
//
//-----------------------------------------------------------------------------

#include "s_sound.h"

#include <algorithm>

#include "Poco/Buffer.h"
#include "c_dispatch.h"
#include "cmdlib.h"
#include "i_system.h"
#include "m_alloc.h"
#include "m_fileio.h"
#include "m_random.h"
#include "mud_includes.h"
#include "oscanner.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#define NORM_PITCH    128
#define NORM_PRIORITY 64
#define NORM_SEP      128

#define S_PITCH_PERTURB 1
#define S_STEREO_SWING  (96 << FRACBITS)

//
// [RH] Print sound debug info. Called from D_Display()
//
void S_NoiseDebug()
{
}

//
// Internals.
//

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
// allocates channel buffer, sets S_sfx lookup.
//
void S_Init(float sfxVolume, float musicVolume)
{
    // [RH] Read in sound sequences
    // NumSequences = 0;
}

void S_Start()
{
}

void S_Stop()
{
}

void S_SoundID(int32_t channel, int32_t sound_id, float volume, int32_t attenuation)
{
}

void S_SoundID(AActor *ent, int32_t channel, int32_t sound_id, float volume, int32_t attenuation)
{
}

void S_SoundID(fixed_t *pt, int32_t channel, int32_t sound_id, float volume, int32_t attenuation)
{
}

void S_LoopedSoundID(AActor *ent, int32_t channel, int32_t sound_id, float volume, int32_t attenuation)
{
}

void S_LoopedSoundID(fixed_t *pt, int32_t channel, int32_t sound_id, float volume, int32_t attenuation)
{
}

// [Russell] - Hack to stop multiple plat stop sounds
void S_PlatSound(fixed_t *pt, int32_t channel, const char *name, float volume, int32_t attenuation)
{
}

void S_Sound(int32_t channel, const char *name, float volume, int32_t attenuation)
{
}

void S_Sound(AActor *ent, int32_t channel, const char *name, float volume, int32_t attenuation)
{
}

void S_Sound(fixed_t *pt, int32_t channel, const char *name, float volume, int32_t attenuation)
{
}

void S_LoopedSound(AActor *ent, int32_t channel, const char *name, float volume, int32_t attenuation)
{
}

void S_LoopedSound(fixed_t *pt, int32_t channel, const char *name, float volume, int32_t attenuation)
{
}

void S_Sound(fixed_t x, fixed_t y, int32_t channel, const char *name, float volume, int32_t attenuation)
{
}

void S_StopSound(fixed_t *pt)
{
}

void S_StopSound(fixed_t *pt, int32_t channel)
{
}

void S_StopSound(AActor *ent, int32_t channel)
{
}

void S_StopAllChannels()
{
}

// Moves all the sounds from one thing to another. If the destination is
// NULL, then the sound becomes a positioned sound.
void S_RelinkSound(AActor *from, AActor *to)
{
}

bool S_GetSoundPlayingInfo(fixed_t *pt, int32_t sound_id)
{
    return false;
}

bool S_GetSoundPlayingInfo(AActor *ent, int32_t sound_id)
{
    return S_GetSoundPlayingInfo(ent ? &ent->x : NULL, sound_id);
}

//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound()
{
}

void S_ResumeSound()
{
}

//
// Updates music & sounds
//
void S_UpdateSounds(void *listener_p)
{
}

void S_UpdateMusic()
{
}

void S_SetMusicVolume(float volume)
{
}

void S_SetSfxVolume(float volume)
{
}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(const char *m_id)
{
}

// [RH] S_ChangeMusic() now accepts the name of the music lump.
// It's up to the caller to figure out what that name is.
void S_ChangeMusic(std::string musicname, int32_t looping)
{
}

void S_StopMusic()
{
}

// [RH] ===============================
//
//	Ambient sound and SNDINFO routines
//
// =============================== [RH]

std::vector<sfxinfo_t>                  S_sfx; // [RH] This is no longer defined in sounds.c
std::map<int32_t, std::vector<int32_t>> S_rnd;

static struct AmbientSound
{
    uint32_t type;                   // type of ambient sound
    int32_t  periodmin;              // # of tics between repeats
    int32_t  periodmax;              // max # of tics for random ambients
    float    volume;                 // relative volume of sound
    float    attenuation;
    char     sound[MAX_SNDNAME + 1]; // Logical name of sound to play
} Ambients[256];

#define RANDOM     1
#define PERIODIC   2
#define CONTINUOUS 3
#define POSITIONAL 4
#define SURROUND   16

void S_HashSounds()
{
    // Mark all buckets as empty
    for (uint32_t i = 0; i < S_sfx.size(); i++)
        S_sfx[i].index = ~0;

    // Now set up the chains
    for (uint32_t i = 0; i < S_sfx.size(); i++)
    {
        const uint32_t j = MakeKey(S_sfx[i].name) % static_cast<uint32_t>(S_sfx.size() - 1);
        S_sfx[i].next    = S_sfx[j].index;
        S_sfx[j].index   = i;
    }
}

int32_t S_FindSound(const char *logicalname)
{
    if (S_sfx.empty())
        return -1;

    int32_t i = S_sfx[MakeKey(logicalname) % static_cast<uint32_t>(S_sfx.size() - 1)].index;

    while ((i != -1) && strnicmp(S_sfx[i].name, logicalname, MAX_SNDNAME))
        i = S_sfx[i].next;

    return i;
}

int32_t S_FindSoundByFilename(const char *filename)
{
    if (filename != NULL)
    {
        for (uint32_t i = 0; i < S_sfx.size(); i++)
            if (S_sfx[i].filename != NULL && strcmp(S_sfx[i].filename, filename) == 0)
                return i;
    }
    return -1;
}

void S_ClearSoundLumps()
{
    S_sfx.clear();
    S_rnd.clear();
}

int32_t FindSoundNoHash(const char *logicalname)
{
    for (size_t i = 0; i < S_sfx.size(); i++)
        if (iequals(logicalname, S_sfx[i].name))
            return i;

    return S_sfx.size();
}

int32_t FindSoundTentative(const char *name)
{
    int32_t id = FindSoundNoHash(name);
    if (id == S_sfx.size())
    {
        id = S_AddSound(name, NULL);
    }
    return id;
}

int32_t S_AddSound(const char *logicalname, const char *filename)
{
    int32_t sfxid = FindSoundNoHash(logicalname);

    // Otherwise, prepare a new one.
    if (sfxid != S_sfx.size())
    {
        sfxinfo_t &sfx = S_sfx[sfxid];

        if (filename != NULL)
            strcpy(sfx.filename, filename);
        sfx.link = sfxinfo_t::NO_LINK;
        if (sfx.israndom)
        {
            S_rnd.erase(sfxid);
            sfx.israndom = false;
        }
    }
    else
    {
        S_sfx.push_back(sfxinfo_t());
        sfxinfo_t &new_sfx = S_sfx[S_sfx.size() - 1];

        // logicalname MUST be < MAX_SNDNAME chars long
        strcpy(new_sfx.name, logicalname);
        new_sfx.data = NULL;
        new_sfx.link = sfxinfo_t::NO_LINK;
        if (filename != NULL)
            strcpy(new_sfx.filename, filename);
        sfxid = S_sfx.size() - 1;
    }

    return sfxid;
}

void S_AddRandomSound(int32_t owner, std::vector<int32_t> &list)
{
    S_rnd[owner]          = list;
    S_sfx[owner].link     = owner;
    S_sfx[owner].israndom = true;
}

// S_ParseSndInfo
void S_ParseSndInfo()
{
    S_ClearSoundLumps();

    if (M_FileExists("lumps/SNDINFO.txt"))
    {
        PHYSFS_File *rawinfo = PHYSFS_openRead("lumps/SNDINFO.txt");

        if (rawinfo == NULL)
            I_Error("Error opening lumps/SNDINFO.txt file");

        uint32_t           filelen = PHYSFS_fileLength(rawinfo);
        Poco::Buffer<char> buffer(filelen);

        if (PHYSFS_readBytes(rawinfo, (void *)buffer.begin(), filelen) != filelen)
        {
            PHYSFS_close(rawinfo);
            I_Error("Error reading lumps/SNDINFO.txt file");
        }

        PHYSFS_close(rawinfo);

        const OScannerConfig config = {
            "SNDINFO", // lumpName
            true,      // semiComments
            true,      // cComments
        };
        OScanner os = OScanner::openBuffer(config, buffer.begin(), buffer.end());

        while (os.scan())
        {
            std::string tok = os.getToken();

            // check if token is a command
            if (tok[0] == '$')
            {
                os.mustScan();
                if (os.compareTokenNoCase("ambient"))
                {
                    // $ambient <num> <logical name> [point [atten]|surround] <type>
                    // [secs] <relative volume>
                    AmbientSound *ambient, dummy;

                    os.mustScanInt();
                    const int32_t index = os.getTokenInt();
                    if (index < 0 || index > 255)
                    {
                        os.warning("Bad ambient index (%d)\n", index);
                        ambient = &dummy;
                    }
                    else
                    {
                        ambient = Ambients + index;
                    }

                    ambient->type      = 0;
                    ambient->periodmin = 0;
                    ambient->periodmax = 0;
                    ambient->volume    = 0.0f;

                    os.mustScan();
                    strncpy(ambient->sound, os.getToken().c_str(), MAX_SNDNAME);
                    ambient->sound[MAX_SNDNAME] = 0;
                    ambient->attenuation        = 0.0f;

                    os.mustScan();
                    if (os.compareTokenNoCase("point"))
                    {
                        ambient->type = POSITIONAL;
                        os.mustScan();

                        if (IsRealNum(os.getToken().c_str()))
                        {
                            ambient->attenuation = (os.getTokenFloat() > 0) ? os.getTokenFloat() : 1;
                            os.mustScan();
                        }
                        else
                        {
                            ambient->attenuation = 1;
                        }
                    }
                    else if (os.compareTokenNoCase("surround"))
                    {
                        ambient->type = SURROUND;
                        os.mustScan();
                        ambient->attenuation = -1;
                    }
                    // else if (os.compareTokenNoCase("world"))
                    //{
                    //  todo
                    //}

                    if (os.compareTokenNoCase("continuous"))
                    {
                        ambient->type |= CONTINUOUS;
                    }
                    else if (os.compareTokenNoCase("random"))
                    {
                        ambient->type |= RANDOM;
                        os.mustScanFloat();
                        ambient->periodmin = static_cast<int32_t>(os.getTokenFloat() * TICRATE);
                        os.mustScanFloat();
                        ambient->periodmax = static_cast<int32_t>(os.getTokenFloat() * TICRATE);
                    }
                    else if (os.compareTokenNoCase("periodic"))
                    {
                        ambient->type |= PERIODIC;
                        os.mustScanFloat();
                        ambient->periodmin = static_cast<int32_t>(os.getTokenFloat() * TICRATE);
                    }
                    else
                    {
                        os.warning("Unknown ambient type (%s)\n", os.getToken().c_str());
                    }

                    os.mustScanFloat();
                    ambient->volume = clamp(os.getTokenFloat(), 0.0f, 1.0f);
                }
                else if (os.compareTokenNoCase("map"))
                {
                    // Hexen-style $MAP command
                    char mapname[8];

                    os.mustScanInt();
                    sprintf(mapname, "MAP%02d", os.getTokenInt());
                    level_pwad_info_t &info = getLevelInfos().findByName(mapname);
                    os.mustScan();
                    if (info.mapname[0])
                    {
                        info.music = os.getToken();
                    }
                }
                else if (os.compareTokenNoCase("alias"))
                {
                    os.mustScan();
                    const int32_t sfxfrom = S_AddSound(os.getToken().c_str(), NULL);
                    os.mustScan();
                    S_sfx[sfxfrom].link = FindSoundTentative(os.getToken().c_str());
                }
                else if (os.compareTokenNoCase("random"))
                {
                    std::vector<int32_t> list;

                    os.mustScan();
                    const int32_t owner = S_AddSound(os.getToken().c_str(), NULL);

                    os.mustScan();
                    os.assertTokenIs("{");
                    while (os.scan() && !os.compareToken("}"))
                    {
                        const int32_t sfxto = FindSoundTentative(os.getToken().c_str());

                        if (owner == sfxto)
                        {
                            os.warning("Definition of random sound '%s' refers to itself "
                                       "recursively.\n",
                                       os.getToken().c_str());
                            continue;
                        }

                        list.push_back(sfxto);
                    }
                    if (list.size() == 1)
                    {
                        // only one sound; treat as alias
                        S_sfx[owner].link = list[0];
                    }
                    else if (list.size() > 1)
                    {
                        S_AddRandomSound(owner, list);
                    }
                }
                else
                {
                    os.warning("Unknown SNDINFO command %s\n", os.getToken().c_str());
                    while (os.scan())
                        if (os.crossed())
                        {
                            os.unScan();
                            break;
                        }
                }
            }
            else
            {
                // token is a logical sound mapping
                char name[MAX_SNDNAME + 1];

                strncpy(name, tok.c_str(), MAX_SNDNAME);
                name[MAX_SNDNAME] = 0;
                os.mustScan();
                S_AddSound(name, os.getToken().c_str());
            }
        }
    }
    S_HashSounds();
}

void A_Ambient(AActor *actor)
{
}

void S_ActivateAmbient(AActor *origin, int32_t ambient)
{
}

VERSION_CONTROL(s_sound_cpp, "$Id: 0c70e151a0a895c0961e9df90b31e95a9cb6ec76 $")
