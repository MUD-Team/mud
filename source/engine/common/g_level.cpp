// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 353def779a2a58b9830d1a1f580b0cdad155b2a2 $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Common level routines
//
//-----------------------------------------------------------------------------

#include <set>

#include "c_console.h"
#include "c_dispatch.h"
#include "d_event.h"
#include "d_main.h"
#include "g_game.h"
#include "gi.h"
#include "i_system.h"
#include "mud_includes.h"
#include "p_acs.h"
#include "p_local.h"
#include "p_saveg.h"
#include "p_unlag.h"
#include "r_common.h"
#include "v_video.h"
#include "w_ident.h"
#include "w_wad.h"

level_locals_t level; // info about current level

level_pwad_info_t g_EmptyLevel;
cluster_info_t    g_EmptyCluster;

//
// LevelInfos methods
//

// Construct from array of levelinfos, ending with an "empty" level
LevelInfos::LevelInfos(const level_info_t *levels) : m_defaultInfos(levels)
{
    // addDefaults();
}

// Destructor frees everything in the class
LevelInfos::~LevelInfos()
{
    clear();
}

// Add default level infos
void LevelInfos::addDefaults()
{
    for (size_t i = 0;; i++)
    {
        const level_info_t &level = m_defaultInfos[i];
        if (!level.exists())
            break;

        // Copied, so it can be mutated.
        level_pwad_info_t info(level);
        m_infos.push_back(info);
    }
}

// Get a specific info index
level_pwad_info_t &LevelInfos::at(size_t i)
{
    return m_infos.at(i);
}

// Clear all cluster definitions
void LevelInfos::clear()
{
    clearSnapshots();
    zapDeferreds();
    m_infos.clear();
}

// Clear all stored snapshots
void LevelInfos::clearSnapshots()
{
    for (_LevelInfoArray::iterator it = m_infos.begin(); it != m_infos.end(); ++it)
    {
        if (it->snapshot)
        {
            delete it->snapshot;
            it->snapshot = NULL;
        }
    }
}

// Add a new levelinfo and return it by reference
level_pwad_info_t &LevelInfos::create()
{
    m_infos.push_back(level_pwad_info_t());
    return m_infos.back();
}

// Find a levelinfo by mapname
level_pwad_info_t &LevelInfos::findByName(const char *mapname)
{
    for (_LevelInfoArray::iterator it = m_infos.begin(); it != m_infos.end(); ++it)
    {
        if (it->mapname == mapname)
        {
            return *it;
        }
    }
    return ::g_EmptyLevel;
}

level_pwad_info_t &LevelInfos::findByName(const std::string &mapname)
{
    for (_LevelInfoArray::iterator it = m_infos.begin(); it != m_infos.end(); ++it)
    {
        if (it->mapname == mapname)
        {
            return *it;
        }
    }
    return ::g_EmptyLevel;
}

level_pwad_info_t &LevelInfos::findByName(const OLumpName &mapname)
{
    for (_LevelInfoArray::iterator it = m_infos.begin(); it != m_infos.end(); ++it)
    {
        if (it->mapname == mapname)
        {
            return *it;
        }
    }
    return ::g_EmptyLevel;
}

// Find a levelinfo by mapnum
level_pwad_info_t &LevelInfos::findByNum(int32_t levelnum)
{
    for (_LevelInfoArray::iterator it = m_infos.begin(); it != m_infos.end(); ++it)
    {
        if (it->levelnum == levelnum && W_CheckNumForName(it->mapname.c_str()) != -1)
        {
            return *it;
        }
    }
    return ::g_EmptyLevel;
}

// Number of info entries.
size_t LevelInfos::size()
{
    return m_infos.size();
}

// Zap all deferred ACS scripts
void LevelInfos::zapDeferreds()
{
    for (_LevelInfoArray::iterator it = m_infos.begin(); it != m_infos.end(); ++it)
    {
        acsdefered_t *def = it->defered;
        while (def)
        {
            acsdefered_t *next = def->next;
            delete def;
            def = next;
        }
        it->defered = NULL;
    }
}

//
// ClusterInfos methods
//

// Construct from array of clusterinfos, ending with an "empty" cluster.
ClusterInfos::ClusterInfos(const cluster_info_t *clusters) : m_defaultInfos(clusters)
{
    // addDefaults();
}

// Destructor frees everything in the class
ClusterInfos::~ClusterInfos()
{
    clear();
}

// Add default level infos
void ClusterInfos::addDefaults()
{
    for (size_t i = 0;; i++)
    {
        const cluster_info_t &cluster = m_defaultInfos[i];
        if (cluster.cluster == 0)
        {
            break;
        }

        // Copied, so it can be mutated.
        m_infos.push_back(cluster);
    }
}

// Get a specific info index
cluster_info_t &ClusterInfos::at(size_t i)
{
    return m_infos.at(i);
}

// Clear all cluster definitions
void ClusterInfos::clear()
{
    return m_infos.clear();
}

// Add a new levelinfo and return it by reference
cluster_info_t &ClusterInfos::create()
{
    m_infos.push_back(cluster_info_t());
    return m_infos.back();
}

// Find a clusterinfo by mapname
cluster_info_t &ClusterInfos::findByCluster(int32_t i)
{
    for (_ClusterInfoArray::iterator it = m_infos.begin(); it != m_infos.end(); ++it)
    {
        if (it->cluster == i)
        {
            return *it;
        }
    }
    return ::g_EmptyCluster;
}

// Number of info entries.
size_t ClusterInfos::size() const
{
    return m_infos.size();
}

void P_RemoveDefereds()
{
    ::getLevelInfos().zapDeferreds();
}

// [ML] Not sure where to put this for now...
// 	G_ParseMusInfo
void G_ParseMusInfo()
{
    // Nothing yet...
}

//
// G_LoadWad
//
// Determines if the vectors of wad & patch filenames differs from the currently
// loaded ones and calls D_DoomWadReboot if so.
//
bool G_LoadWad(const OWantFiles &newwadfiles, const std::string &mapname)
{
    bool AddedIWAD = false;
    bool Reboot    = false;

    // Did we pass an IWAD?
    if (!newwadfiles.empty() && W_IsKnownIWAD(newwadfiles[0]))
    {
        AddedIWAD = true;
    }

    // Check our environment, if the same WADs are used, ignore this command.

    // Did we switch IWAD files?
    if (AddedIWAD && !::wadfiles.empty())
    {
        if (newwadfiles.at(0).getBasename() != wadfiles.at(0).getBasename())
        {
            Reboot = true;
        }
    }

    // Do the sizes of the WAD lists not match up?
    if (!Reboot)
    {
        if (::wadfiles.size() != newwadfiles.size())
        {
            Reboot = true;
        }
    }

    // Do our WAD lists match up exactly?
    if (!Reboot)
    {
        for (size_t i = 0; i < ::wadfiles.size() && i < newwadfiles.size(); i++)
        {
            if (!(newwadfiles.at(i).getBasename() == ::wadfiles.at(i).getBasename()))
            {
                Reboot = true;
                break;
            }
        }
    }

    if (Reboot)
    {
        unnatural_level_progression = true;

        D_DoomWadReboot(newwadfiles);
        if (!missingfiles.empty())
        {
            G_DeferedInitNew(startmap);
            return false;
        }
    }

    if (mapname.length())
    {
        if (W_CheckNumForName(mapname.c_str()) != -1)
        {
            G_DeferedInitNew(mapname.c_str());
        }
        else
        {
            Printf_Bold("map %s not found, loading start map instead", mapname.c_str());
            G_DeferedInitNew(startmap);
        }
    }
    else
        G_DeferedInitNew(startmap);

    return true;
}

const char *ParseString2(const char *data);

//
// G_LoadWadString
//
// Takes a string of random wads and patches, which is sorted through and
// trampolined to the implementation of G_LoadWad.
//
bool G_LoadWadString(const std::string &str, const std::string &mapname)
{
    OWantFiles newwadfiles;

    const char *data = str.c_str();
    for (size_t argv = 0; (data = ParseString2(data)); argv++)
    {
        OWantFile file;
        if (!OWantFile::make(file, ::com_token))
        {
            Printf(PRINT_WARNING, "Could not parse \"%s\" into file, skipping...\n", ::com_token);
            continue;
        }

        // Just push the unknown file into the resource list.
        newwadfiles.push_back(file);
        continue;
    }

    return G_LoadWad(newwadfiles, mapname);
}

BEGIN_COMMAND(map)
{
    if (argc > 1)
    {
        char mapname[32];

        // [Dash|RD] -- We can make a safe assumption that the user might not specify
        //              the whole lumpname for the level, and might opt for just the
        //              number. This makes sense, so why isn't there any code for it?
        if (W_CheckNumForName(argv[1]) == -1 && isdigit(argv[1][0]))
        { // The map name isn't valid, so lets try to make some assumptions for the user.

            // If argc is 2, we assume Doom 2/Final Doom. If it's 3, Ultimate Doom.
            // [Russell] - gamemode is always the better option compared to above
            if (argc == 2)
            {
                sprintf(mapname, "MAP%02i", atoi(argv[1]));
            }

            if (W_CheckNumForName(mapname) == -1)
            { // Still no luck, oh well.
                Printf(PRINT_WARNING, "Map %s not found.\n", argv[1]);
            }
            else
            { // Success
                unnatural_level_progression = true;
                G_DeferedInitNew(mapname);
            }
        }
        else
        {
            // Ch0wW - Map was still not found, so don't bother trying loading the map.
            if (W_CheckNumForName(argv[1]) == -1)
            {
                Printf(PRINT_WARNING, "Map %s not found.\n", argv[1]);
            }
            else
            {
                unnatural_level_progression = true;
                uppercopy(mapname, argv[1]); // uppercase the mapname
                G_DeferedInitNew(mapname);
            }
        }
    }
    else
    {
        Printf(PRINT_HIGH, "The current map is %s: \"%s\"\n", level.mapname.c_str(), level.level_name);
    }
}
END_COMMAND(map)

char *CalcMapName(int32_t episode, int32_t level)
{
    static char lumpname[9];
    sprintf(lumpname, "MAP%02d", level);
    return lumpname;
}

void G_AirControlChanged()
{
    if (level.aircontrol <= 256)
    {
        level.airfriction = FRACUNIT;
    }
    else
    {
        // Friction is inversely proportional to the amount of control
        float fric        = ((float)level.aircontrol / 65536.f) * -0.0941f + 1.0004f;
        level.airfriction = (fixed_t)(fric * 65536.f);
    }
}

// Serialize or unserialize the state of the level depending on the state of
// the first parameter.  Second parameter is true if you need to deal with hub
// playerstate.  Third parameter is true if you want to handle playerstate
// yourself (map resets), just make sure you set it the same for both
// serialization and unserialization.
void G_SerializeLevel(FArchive &arc, bool hubLoad)
{
    if (arc.IsStoring())
    {
        uint32_t playernum = players.size();
        arc << level.flags << level.fadeto_color[0] << level.fadeto_color[1] << level.fadeto_color[2]
            << level.fadeto_color[3] << level.found_secrets << level.found_items << level.killed_monsters
            << level.gravity << level.aircontrol;

        G_AirControlChanged();

        for (int32_t i = 0; i < NUM_MAPVARS; i++)
            arc << level.vars[i];

        if (!arc.IsReset())
            arc << playernum;
    }
    else
    {
        uint32_t playernum;
        arc >> level.flags >> level.fadeto_color[0] >> level.fadeto_color[1] >> level.fadeto_color[2] >>
            level.fadeto_color[3] >> level.found_secrets >> level.found_items >> level.killed_monsters >>
            level.gravity >> level.aircontrol;

        G_AirControlChanged();

        for (int32_t i = 0; i < NUM_MAPVARS; i++)
            arc >> level.vars[i];

        if (!arc.IsReset())
        {
            arc >> playernum;
            players.resize(playernum);
        }
    }

    if (!hubLoad && !arc.IsReset())
        P_SerializePlayers(arc);

    P_SerializeThinkers(arc, hubLoad);
    P_SerializeWorld(arc);
    P_SerializePolyobjs(arc);
    P_SerializeSounds(arc);
}

// Archives the current level
void G_SnapshotLevel()
{
    delete level.info->snapshot;

    level.info->snapshot = new FLZOMemFile;
    level.info->snapshot->Open();

    FArchive arc(*level.info->snapshot);

    G_SerializeLevel(arc, false);
}

// Unarchives the current level based on its snapshot
// The level should have already been loaded and setup.
void G_UnSnapshotLevel(bool hubLoad)
{
    if (level.info->snapshot == NULL)
        return;

    level.info->snapshot->Reopen();
    FArchive arc(*level.info->snapshot);
    if (hubLoad)
        arc.SetHubTravel(); // denis - hexen?
    G_SerializeLevel(arc, hubLoad);
    arc.Close();
    // No reason to keep the snapshot around once the level's been entered.
    delete level.info->snapshot;
    level.info->snapshot = NULL;
}

void G_ClearSnapshots()
{
    getLevelInfos().clearSnapshots();
}

static void writeSnapShot(FArchive &arc, level_pwad_info_t &info)
{
    arc.Write(info.mapname.c_str(), 8);
    info.snapshot->Serialize(arc);
}

void G_SerializeSnapshots(FArchive &arc)
{
    LevelInfos &levels = getLevelInfos();

    if (arc.IsStoring())
    {
        for (size_t i = 0; i < levels.size(); i++)
        {
            level_pwad_info_t &level = levels.at(i);
            if (level.snapshot)
            {
                writeSnapShot(arc, level);
            }
        }

        // Signal end of snapshots
        arc << static_cast<char>(0);
    }
    else
    {
        LevelInfos &levels = getLevelInfos();
        char        mapname[8];

        G_ClearSnapshots();

        arc >> mapname[0];
        while (mapname[0])
        {
            arc.Read(&mapname[1], 7);

            // FIXME: We should really serialize the full levelinfo
            level_pwad_info_t &info = levels.findByName(mapname);
            info.snapshot           = new FLZOMemFile;
            info.snapshot->Serialize(arc);
            arc >> mapname[0];
        }
    }
}

static void writeDefereds(FArchive &arc, level_pwad_info_t &info)
{
    arc.Write(info.mapname.c_str(), 8);
    arc << info.defered;
}

void P_SerializeACSDefereds(FArchive &arc)
{
    LevelInfos &levels = getLevelInfos();

    if (arc.IsStoring())
    {
        for (size_t i = 0; i < levels.size(); i++)
        {
            level_pwad_info_t &level = levels.at(i);
            if (level.defered)
            {
                writeDefereds(arc, level);
            }
        }

        // Signal end of defereds
        arc << (uint8_t)0;
    }
    else
    {
        LevelInfos &levels = getLevelInfos();
        char        mapname[8];

        P_RemoveDefereds();

        arc >> mapname[0];
        while (mapname[0])
        {
            arc.Read(&mapname[1], 7);
            level_pwad_info_t &info = levels.findByName(mapname);
            if (!info.exists())
            {
                char name[9];
                strncpy(name, mapname, ARRAY_LENGTH(name) - 1);
                name[8] = 0;
                I_Error("Unknown map '%s' in savegame", name);
            }
            arc >> info.defered;
            arc >> mapname[0];
        }
    }
}

static int32_t startpos; // [RH] Support for multiple starts per level

void G_DoWorldDone()
{
    gamestate = GS_LEVEL;
    if (wminfo.next[0] == 0)
    {
        // Don't die if no next map is given,
        // just repeat the current one.
        Printf(PRINT_WARNING, "No next map specified.\n");
    }
    else
    {
        level.mapname = wminfo.next;
    }
    G_DoLoadLevel(startpos);
    startpos   = 0;
    gameaction = ga_nothing;
    viewactive = true;
}

extern dyncolormap_t NormalLight;

EXTERN_CVAR(sv_gravity)
EXTERN_CVAR(sv_aircontrol)

void G_InitLevelLocals()
{
    uint8_t old_fadeto_color[4];
    memcpy(old_fadeto_color, level.fadeto_color, 4);

    R_ExitLevel();

    level.gravity    = sv_gravity;
    level.aircontrol = static_cast<fixed_t>(sv_aircontrol * 65536.f);
    G_AirControlChanged();

    // clear all ACS variables
    memset(level.vars, 0, sizeof(level.vars));

    // Get our canonical level data.
    level_pwad_info_t &info = getLevelInfos().findByName(::level.mapname);

    // [ML] 5/11/06 - Remove sky scrolling and sky2
    // [SL] 2012-03-19 - Add sky2 back
    ::level.info    = (level_info_t *)&info;
    ::level.skypic2 = info.skypic2;
    memcpy(::level.fadeto_color, info.fadeto_color, 4);

    memcpy(::level.outsidefog_color, info.outsidefog_color, 4);

    ::level.flags |= LEVEL_DEFINEDINMAPINFO;
    if (info.gravity != 0.f)
    {
        ::level.gravity = info.gravity;
    }
    if (info.aircontrol != 0.f)
    {
        ::level.aircontrol = static_cast<fixed_t>(info.aircontrol * 65536.f);
    }

    ::level.partime  = info.partime;
    ::level.cluster  = info.cluster;
    ::level.flags    = info.flags;
    ::level.levelnum = info.levelnum;
    ArrayCopy(::level.level_fingerprint, info.level_fingerprint);

    // Only copy the level name if there's a valid level name to be copied.

    if (!info.level_name.empty())
    {
        // Get rid of initial lump name or level number.
        std::string begin;
        if (info.mapname[0] == 'E' && info.mapname[2] == 'M')
        {
            std::string search;
            StrFormat(search, "E%cM%c: ", info.mapname[1], info.mapname[3]);

            const std::size_t pos = info.level_name.find(search);

            if (pos != std::string::npos)
                begin = info.level_name.substr(pos + search.length());
            else
                begin = info.level_name;
        }
        else if (strstr(info.mapname.c_str(), "MAP") == &info.mapname[0])
        {
            std::string search;
            StrFormat(search, "%u: ", info.levelnum);

            const std::size_t pos = info.level_name.find(search);

            if (pos != std::string::npos)
                begin = info.level_name.substr(pos + search.length());
            else
                begin = info.level_name;
        }
        else
        {
            begin = info.level_name;
        }

        if (!begin.empty())
        {
            std::string level_name(begin);
            TrimString(level_name);
            strncpy(::level.level_name, level_name.c_str(), ARRAY_LENGTH(::level.level_name) - 1);
        }
        else
        {
            strncpy(::level.level_name, "Untitled Level", ARRAY_LENGTH(::level.level_name) - 1);
        }
    }
    else
    {
        strncpy(::level.level_name, "Untitled Level", ARRAY_LENGTH(::level.level_name) - 1);
    }

    ::level.nextmap   = info.nextmap;
    ::level.secretmap = info.secretmap;
    ::level.music     = info.music;
    ::level.skypic    = info.skypic;
    if (!::level.skypic2[0])
    {
        ::level.skypic2 = ::level.skypic.c_str();
    }

    //	memset (level.vars, 0, sizeof(level.vars));

    if (memcmp(::level.fadeto_color, old_fadeto_color, 4) != 0)
    {
        V_RefreshColormaps();
    }

    ::level.exitpic  = info.exitpic;
    ::level.enterpic = info.enterpic;
    ::level.endpic   = info.endpic;

    ::level.intertext       = info.intertext;
    ::level.intertextsecret = info.intertextsecret;
    ::level.interbackdrop   = info.interbackdrop;
    ::level.intermusic      = info.intermusic;

    ::level.bossactions = info.bossactions;

    ::level.detected_gametype = GM_COOP;

    movingsectors.clear();
}

static void MapinfoHelp()
{
    Printf(PRINT_HIGH, "mapinfo - Looks up internal information about levels\n\n"
                       "Usage:\n"
                       "  ] mapinfo mapname <LUMPNAME>\n"
                       "  Looks up a map contained in the lump LUMPNAME.\n\n"
                       "  ] mapinfo levelnum <LEVELNUM>\n"
                       "  Looks up a map with a levelnum of LEVELNUM.\n\n"
                       "  ] mapinfo at <LEVELINFO ID>\n"
                       "  Looks up a map based on its placement in the internal level info array.\n\n"
                       "  ] mapinfo size\n"
                       "  Return the size of the internal level info array.\n");
}

// A debugging tool to examine the state of computed map data.
BEGIN_COMMAND(mapinfo)
{
    if (argc < 2)
    {
        MapinfoHelp();
        return;
    }

    LevelInfos &levels = getLevelInfos();
    if (stricmp(argv[1], "size") == 0)
    {
        Printf(PRINT_HIGH, "%zu maps found\n", levels.size());
        return;
    }

    if (argc < 3)
    {
        MapinfoHelp();
        return;
    }

    level_pwad_info_t *infoptr = NULL;
    if (stricmp(argv[1], "mapname") == 0)
    {
        infoptr = &levels.findByName(argv[2]);
        if (!infoptr->exists())
        {
            Printf(PRINT_HIGH, "Map \"%s\" not found\n", argv[2]);
            return;
        }
    }
    else if (stricmp(argv[1], "levelnum") == 0)
    {
        int32_t levelnum = atoi(argv[2]);
        infoptr          = &levels.findByNum(levelnum);
        if (!infoptr->exists())
        {
            Printf(PRINT_HIGH, "Map number %d not found\n", levelnum);
            return;
        }
    }
    else if (stricmp(argv[1], "at") == 0)
    {
        // Check ahead of time, otherwise we might crash.
        int32_t id = atoi(argv[2]);
        if (id < 0 || id >= levels.size())
        {
            Printf(PRINT_HIGH, "Map index %d does not exist\n", id);
            return;
        }
        infoptr = &levels.at(id);
    }
    else
    {
        MapinfoHelp();
        return;
    }

    level_pwad_info_t &info = *infoptr;

    Printf(PRINT_HIGH, "Map Name: %s\n", info.mapname.c_str());
    Printf(PRINT_HIGH, "Level Number: %d\n", info.levelnum);
    Printf(PRINT_HIGH, "Level Name: %s\n", info.level_name.c_str());
    Printf(PRINT_HIGH, "Intermission Graphic: %s\n", info.pname.c_str());
    Printf(PRINT_HIGH, "Next Map: %s\n", info.nextmap.c_str());
    Printf(PRINT_HIGH, "Secret Map: %s\n", info.secretmap.c_str());
    Printf(PRINT_HIGH, "Par Time: %d\n", info.partime);
    Printf(PRINT_HIGH, "Sky: %s\n", info.skypic.c_str());
    Printf(PRINT_HIGH, "Music: %s\n", info.music.c_str());

    // Stringify the set level flags.
    std::string flags;
    flags += (info.flags & LEVEL_DOUBLESKY ? " DOUBLESKY" : "");
    flags += (info.flags & LEVEL_NOSOUNDCLIPPING ? " NOSOUNDCLIPPING" : "");
    flags += (info.flags & LEVEL_MAP07SPECIAL ? " MAP07SPECIAL" : "");
    flags += (info.flags & LEVEL_BRUISERSPECIAL ? " BRUISERSPECIAL" : "");
    flags += (info.flags & LEVEL_CYBORGSPECIAL ? " CYBORGSPECIAL" : "");
    flags += (info.flags & LEVEL_SPIDERSPECIAL ? " SPIDERSPECIAL" : "");
    flags += (info.flags & LEVEL_SPECLOWERFLOOR ? " SPECLOWERFLOOR" : "");
    flags += (info.flags & LEVEL_SPECOPENDOOR ? " SPECOPENDOOR" : "");
    flags += (info.flags & LEVEL_SPECACTIONSMASK ? " SPECACTIONSMASK" : "");
    flags += (info.flags & LEVEL_MONSTERSTELEFRAG ? " MONSTERSTELEFRAG" : "");
    flags += (info.flags & LEVEL_EVENLIGHTING ? " EVENLIGHTING" : "");
    flags += (info.flags & LEVEL_SNDSEQTOTALCTRL ? " SNDSEQTOTALCTRL" : "");
    flags += (info.flags & LEVEL_FORCENOSKYSTRETCH ? " FORCENOSKYSTRETCH" : "");
    flags += (info.flags & LEVEL_STARTLIGHTNING ? " STARTLIGHTNING" : "");
    flags += (info.flags & LEVEL_FILTERSTARTS ? " FILTERSTARTS" : "");
    flags += (info.flags & LEVEL_LOBBYSPECIAL ? " LOBBYSPECIAL" : "");
    flags += (info.flags & LEVEL_USEPLAYERSTARTZ ? " USEPLAYERSTARTZ" : "");
    flags += (info.flags & LEVEL_DEFINEDINMAPINFO ? " DEFINEDINMAPINFO" : "");
    flags += (info.flags & LEVEL_CHANGEMAPCHEAT ? " CHANGEMAPCHEAT" : "");
    flags += (info.flags & LEVEL_VISITED ? " VISITED" : "");

    if (flags.length() > 0)
    {
        Printf(PRINT_HIGH, "Flags:%s\n", flags.c_str());
    }
    else
    {
        Printf(PRINT_HIGH, "Flags: None\n");
    }

    Printf(PRINT_HIGH, "Cluster: %d\n", info.cluster);
    Printf(PRINT_HIGH, "Snapshot? %s\n", info.snapshot ? "Yes" : "No");
    Printf(PRINT_HIGH, "ACS defereds? %s\n", info.defered ? "Yes" : "No");
}
END_COMMAND(mapinfo)

// A debugging tool to examine the state of computed cluster data.
BEGIN_COMMAND(clusterinfo)
{
    if (argc < 2)
    {
        Printf(PRINT_HIGH, "Usage: clusterinfo <cluster id>\n");
        return;
    }

    cluster_info_t &info = getClusterInfos().findByCluster(std::atoi(argv[1]));
    if (info.cluster == 0)
    {
        Printf(PRINT_HIGH, "Cluster %s not found\n", argv[1]);
        return;
    }

    Printf(PRINT_HIGH, "Cluster: %d\n", info.cluster);

    // Stringify the set cluster flags.
    std::string flags;
    flags += (info.flags & CLUSTER_HUB ? " HUB" : "");

    if (flags.length() > 0)
    {
        Printf(PRINT_HIGH, "Flags:%s\n", flags.c_str());
    }
    else
    {
        Printf(PRINT_HIGH, "Flags: None\n");
    }
}
END_COMMAND(clusterinfo)

// Get global canonical levelinfo
LevelInfos &getLevelInfos()
{
    static LevelInfos li(NULL);
    return li;
}

// Get global canonical clusterinfo
ClusterInfos &getClusterInfos()
{
    static ClusterInfos ci(NULL);
    return ci;
}

VERSION_CONTROL(g_level_cpp, "$Id: 353def779a2a58b9830d1a1f580b0cdad155b2a2 $")
