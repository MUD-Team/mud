//----------------------------------------------------------------------------
//  EDGE Player Handling
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2024 The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "g_game.h"

#include <limits.h>
#include <string.h>
#include <time.h>

#include "bot_think.h"
#include "con_main.h"
#include "dm_state.h"
#include "dstrings.h"
#include "epi.h"
#include "e_input.h"
#include "e_main.h"
#include "epi_endian.h"
#include "epi_filesystem.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "i_system.h"
#include "m_cheat.h"
#include "m_random.h"
#include "n_network.h"
#include "p_setup.h"
#include "p_tick.h"
#include "r_colormap.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_sky.h"
#include "s_music.h"
#include "s_sound.h"
#include "sv_main.h"
#include "version.h"
#include "w_files.h"

extern ConsoleVariable double_framerate;

GameState game_state = kGameStateNothing;

GameAction game_action = kGameActionNothing;

bool paused        = false;

int key_pause;

// if true, load all graphics at start
bool precache = true;

// -KM- 1998/11/25 Exit time is the time when the level will actually finish
// after hitting the exit switch/killing the boss.  So that you see the
// switch change or the boss die.

int  exit_time     = INT_MAX;
bool exit_skip_all = false; // -AJA- temporary (maybe become "exit_mode")
int  exit_hub_tag  = 0;

int key_show_players;

// GAMEPLAY MODES:
//
//   numplayers  deathmatch   mode
//   --------------------------------------
//     <= 1         0         single player
//     >  1         0         coop
//     -            1         deathmatch
//     -            2         altdeath

int deathmatch;

SkillLevel game_skill = kSkillMedium;

// -ACB- 2004/05/25 We need to store our current/next mapdefs
const MapDefinition *current_map = nullptr;
const MapDefinition *next_map    = nullptr;

int                  current_hub_tag = 0; // affects where players are spawned
const MapDefinition *current_hub_first;   // first map in group of hubs

// -KM- 1998/12/16 These flags hold everything needed about a level
GameFlags level_flags;

//--------------------------------------------

// deferred stuff...
static NewGameParameters *defer_params = nullptr;

static void DoNewGame(void);
static void DoLoadGame(void);
static void DoCompleted(void);
static void DoSaveGame(void);
static void DoEndGame(void);

static void InitNew(NewGameParameters &params);
static void RespawnPlayer(Player *p);
static void SpawnInitialPlayers(void);

static bool LoadGameFromFile(std::string filename, bool is_hub = false);
static bool SaveGameToFile(std::string filename, const char *description);

static void HandleLevelFlag(bool *special, MapFlag flag)
{
    if (current_map->force_on_ & (flag))
        *special = true;
    else if (current_map->force_off_ & (flag))
        *special = false;
}

void LoadLevel_Bits(void)
{
    if (current_map == nullptr)
        FatalError("DoLoadLevel: No Current Map selected");

    // Set the sky map.
    //
    // First thing, we have a dummy sky texture name, a flat. The data is
    // in the WAD only because we look for an actual index, instead of simply
    // setting one.
    //
    // -ACB- 1998/08/09 Reference current map for sky name.

    sky_image = ImageLookup(current_map->sky_.c_str(), kImageNamespaceTexture);

    game_state = kGameStateNothing; // FIXME: needed ???

    // -AJA- FIXME: this background camera stuff is a mess
    background_camera_map_object = nullptr;

    for (int pnum = 0; pnum < kMaximumPlayers; pnum++)
    {
        Player *p = players[pnum];
        if (!p)
            continue;

        if (p->player_state_ == kPlayerDead || (current_map->force_on_ & kMapFlagResetPlayer))
        {
            p->player_state_ = kPlayerAwaitingRespawn;
        }

        p->frags_ = 0;
    }

    // -KM- 1998/12/16 Make map flags actually do stuff.
    // -AJA- 2000/02/02: Made it more generic.
    HandleLevelFlag(&level_flags.items_respawn, kMapFlagItemRespawn);
    HandleLevelFlag(&level_flags.fast_monsters, kMapFlagFastParm);
    HandleLevelFlag(&level_flags.more_blood, kMapFlagMoreBlood);
    HandleLevelFlag(&level_flags.cheats, kMapFlagCheats);
    HandleLevelFlag(&level_flags.enemies_respawn, kMapFlagRespawn);
    HandleLevelFlag(&level_flags.enemy_respawn_mode, kMapFlagResRespawn);
    HandleLevelFlag(&level_flags.limit_zoom, kMapFlagLimitZoom);
    HandleLevelFlag(&level_flags.kicking, kMapFlagKicking);
    HandleLevelFlag(&level_flags.weapon_switch, kMapFlagWeaponSwitch);
    HandleLevelFlag(&level_flags.team_damage, kMapFlagTeamDamage);

    if (current_map->force_on_ & kMapFlagAutoAim)
        level_flags.autoaim = kAutoAimOn;
    else if (current_map->force_off_ & kMapFlagAutoAim)
        level_flags.autoaim = kAutoAimOff;

    //
    // Note: It should be noted that only the game_skill is
    // passed as the level is already defined in current_map,
    // The method for changing current_map, is using by
    // DeferredNewGame.
    //
    // -ACB- 1998/08/09 New LevelSetup
    // -KM- 1998/11/25 LevelSetup accepts the autotag
    //    

    for (int pnum = 0; pnum < kMaximumPlayers; pnum++)
    {
        Player *p = players[pnum];
        if (!p)
            continue;

        p->kill_count_ = p->secret_count_ = p->item_count_ = 0;
        p->map_object_                                     = nullptr;
    }

    // Initial height of PointOfView will be set by player think.
    players[console_player]->view_z_ = kFloatUnused;

    level_time_elapsed = 0;

    LevelSetup();

    exit_time     = INT_MAX;
    exit_skip_all = false;
    exit_hub_tag  = 0;

    BotBeginLevel();

    game_state = kGameStateLevel;

    SetConsoleVisible(kConsoleVisibilityNotVisible);

    // clear cmd building stuff
    ClearEventInput();

    paused = false;
}

//
// REQUIRED STATE:
//   (a) current_map
//   (b) current_hub_tag
//   (c) players[], numplayers (etc)
//   (d) game_skill + deathmatch
//   (e) level_flags
//
//   ??  exit_time
//
void DoLoadLevel(void)
{
    if (current_hub_tag == 0)
        SaveClearSlot("current");

    if (current_hub_tag > 0)
    {
        // HUB system: check for loading a previously visited map
        const char *mapname = SaveMapName(current_map);

        std::string fn(SaveFilename("current", mapname));

        if (epi::FileExists(fn))
        {
            LogPrint("Loading HUB...\n");

            if (!LoadGameFromFile(fn, true))
                FatalError("LOAD-HUB failed with filename: %s\n", fn.c_str());

            SpawnInitialPlayers();

            // Need to investigate if LuaBeginLevel() needs to go here too now
            // - Dasho

            RemoveOldAvatars();

            HubFastForward();
            return;
        }
    }

    LoadLevel_Bits();

    SpawnInitialPlayers();    
}

//
// GameResponder
//
// Get info needed to make ticcmd_ts for the players.
//
bool GameResponder(InputEvent *ev)
{
    if (ev->type == kInputEventKeyDown && CheckKeyMatch(key_show_players, ev->value.key.sym))
    {
        if (game_state == kGameStateLevel) //!!!! && !InDeathmatch())
        {
            ToggleDisplayPlayer();
            return true;
        }
    }

    if (!network_game && ev->type == kInputEventKeyDown && CheckKeyMatch(key_pause, ev->value.key.sym))
    {
        paused = !paused;

        if (paused)
        {
            PauseMusic();
            PauseSound();
        }
        else
        {
            ResumeMusic();
            ResumeSound();
        }

        return true;
    }

    if (game_state == kGameStateLevel)
    {
        if (CheatResponder(ev))
            return true; // cheat code at it
    }

    return InputResponder(ev);
}

static void CheckPlayersReborn(void)
{
    for (int pnum = 0; pnum < kMaximumPlayers; pnum++)
    {
        Player *p = players[pnum];

        if (!p || p->player_state_ != kPlayerAwaitingRespawn)
            continue;

        if (InSinglePlayerMatch())
        {
            // reload the level
            game_action = kGameActionLoadLevel;

            // -AJA- if we are on a HUB map, then we must go all the
            //       way back to the beginning.
            if (current_hub_first)
            {
                current_map       = current_hub_first;
                current_hub_tag   = 0;
                current_hub_first = nullptr;
            }
            return;
        }

        RespawnPlayer(p);
    }
}

void DoBigGameStuff(void)
{
    // do things to change the game state
    while (game_action != kGameActionNothing)
    {
        GameAction action = game_action;
        game_action       = kGameActionNothing;

        switch (action)
        {
        case kGameActionNewGame:
            DoNewGame();
            break;

        case kGameActionLoadLevel:
            DoLoadLevel();
            break;

        case kGameActionLevelCompleted:
            DoCompleted();
            break;

        case kGameActionLoadGame:
            DoLoadGame();
            break;

        case kGameActionSaveGame:
            DoSaveGame();
            break;

        case kGameActionEndGame:
            DoEndGame();
            break;

        default:
            FatalError("DoBigGameStuff: Unknown game_action %d", game_action);
            break;
        }
    }
}

void GameTicker(void)
{
    bool extra_tic = (game_tic & 1) == 1;

    if (extra_tic && double_framerate.d_)
    {
        switch (game_state)
        {
        case kGameStateLevel:
            // get commands
            GrabTicCommands();

            //!!!  MapObjectTicker();
            MapObjectTicker(true);
            break;

        default:
            break;
        }
        return;
    }

    // ANIMATE FLATS AND TEXTURES GLOBALLY
    AnimationTicker();

    // do main actions
    switch (game_state)
    {

    case kGameStateLevel:
        // get commands
        GrabTicCommands();

        MapObjectTicker(false);
        
        // do player reborns if needed
        CheckPlayersReborn();
        break;

    default:
        break;
    }
}

static void RespawnPlayer(Player *p)
{
    // first disassociate the corpse (if any)
    if (p->map_object_)
        p->map_object_->player_ = nullptr;

    p->map_object_ = nullptr;

    // spawn at random spot if in death match
    if (InDeathmatch())
        DeathMatchSpawnPlayer(p);
    else if (current_hub_tag > 0)
        GameHubSpawnPlayer(p, current_hub_tag);
    else
        CoopSpawnPlayer(p); // respawn at the start
}

static void SpawnInitialPlayers(void)
{
    LogDebug("Deathmatch %d\n", deathmatch);

    // spawn the active players
    for (int pnum = 0; pnum < kMaximumPlayers; pnum++)
    {
        Player *p = players[pnum];
        if (p == nullptr)
        {
            // no real player, maybe spawn a helper dog?
            SpawnHelper(pnum);
            continue;
        }

        RespawnPlayer(p);
    }

    // check for missing player start.
    if (players[console_player]->map_object_ == nullptr)
        FatalError("Missing player start !\n");

    SetDisplayPlayer(console_player); // view the guy you are playing
}

void DeferredScreenShot(void)
{
    m_screenshot_required = true;
}

// -KM- 1998/11/25 Added time param which is the time to wait before
//  actually exiting level.
void ExitLevel(int time)
{
    next_map      = LookupMap(current_map->next_mapname_.c_str());
    exit_time     = level_time_elapsed + time;
    exit_skip_all = false;
    exit_hub_tag  = 0;
}

// -ACB- 1998/08/08 We don't have support for the german edition
//                  removed the check for map31.
void ExitLevelSecret(int time)
{
    next_map      = LookupMap(current_map->secretmapname_.c_str());
    exit_time     = level_time_elapsed + time;
    exit_skip_all = false;
    exit_hub_tag  = 0;
}

void ExitToLevel(char *name, int time, bool skip_all)
{
    next_map      = LookupMap(name);
    exit_time     = level_time_elapsed + time;
    exit_skip_all = skip_all;
    exit_hub_tag  = 0;
}

void ExitToHub(const char *map_name, int tag)
{
    if (tag <= 0)
        FatalError("Hub exit line/command: bad tag %d\n", tag);

    next_map = LookupMap(map_name);
    if (!next_map)
        FatalError("ExitToHub: No such map %s !\n", map_name);

    exit_time     = level_time_elapsed + 5;
    exit_skip_all = true;
    exit_hub_tag  = tag;
}

void ExitToHub(int map_number, int tag)
{
    EPI_ASSERT(current_map);

    char name_buf[32];

    // bit hackish: decided whether to use MAP## or E#M#
    if (current_map->name_[0] == 'E')
    {
        sprintf(name_buf, "E%dM%d", 1 + (map_number / 10), map_number % 10);
    }
    else
        sprintf(name_buf, "MAP%02d", map_number);

    ExitToHub(name_buf, tag);
}

//
// REQUIRED STATE:
//   (a) current_map, next_map
//   (b) players[]
//   (c) level_time_elapsed
//   (d) exit_skip_all
//   (d) exit_hub_tag
//   (e) intermission_stats.kills (etc)
//
static void DoCompleted(void)
{
    EPI_ASSERT(current_map);

    exit_time = INT_MAX;

    for (int pnum = 0; pnum < kMaximumPlayers; pnum++)
    {
        Player *p = players[pnum];
        if (!p)
            continue;

        p->level_time_ = level_time_elapsed;

        // take away cards and stuff
        PlayerFinishLevel(p, exit_hub_tag > 0);
    }

    BotEndLevel();

    // Hard coding whilst we figure out how map traversal will work
    exit_skip_all = true;

    // handle "no stat" levels
    if (current_map->wistyle_ == kIntermissionStyleNone || exit_skip_all)
    {
        if (exit_skip_all && next_map)
        {
            if (exit_hub_tag <= 0)
                current_hub_first = nullptr;
            else
            {
                // save current map for HUB system
                LogPrint("Saving HUB...\n");

                // remember avatars of players, so we can remove them
                // when we return to this level.
                MarkPlayerAvatars();

                const char *mapname = SaveMapName(current_map);

                std::string fn(SaveFilename("current", mapname));

                if (!SaveGameToFile(fn, "__HUB_SAVE__"))
                    FatalError("SAVE-HUB failed with filename: %s\n", fn.c_str());

                if (!current_hub_first)
                    current_hub_first = current_map;
            }

            current_map     = next_map;
            current_hub_tag = exit_hub_tag;

            game_action = kGameActionLoadLevel;
        }
        else
        {
            FatalError("DoCompleted: Transition to finale not supported");
        }

        return;
    }

    FatalError("DoCompleted: Transition to intermission not supported");

}

void DeferredLoadGame(int slot)
{
// Stubbed out pending save system rework - Dasho
    return;
}

static bool LoadGameFromFile(std::string filename, bool is_hub)
{
// Stubbed out pending save system rework - Dasho
    return false;
}

//
// REQUIRED STATE:
//   (a) defer_load_slot
//
//   ?? nothing else ??
//
static void DoLoadGame(void)
{
// Stubbed out pending save system rework - Dasho
    return;
}

//
// DeferredSaveGame
//
// Called by the menu task.
// Description is a 24 byte text string
//
void DeferredSaveGame(int slot, const char *description)
{
// Stubbed out pending save system rework - Dasho
    return;
}

static bool SaveGameToFile(std::string filename, const char *description)
{
// Stubbed out pending save system rework - Dasho
    return false;
}

static void DoSaveGame(void)
{
// Stubbed out pending save system rework - Dasho
    return;
}

//
// GameInitNew Stuff
//
// Can be called by the startup code or the menu task.
// consoleplayer, displayplayer, players[] are setup.
//

//---> newgame_params_c class

NewGameParameters::NewGameParameters()
    : skill_(kSkillMedium), deathmatch_(0), map_(nullptr), random_seed_(0), total_players_(0), flags_(nullptr)
{
    for (int i = 0; i < kMaximumPlayers; i++)
    {
        players_[i] = kPlayerFlagNoPlayer;
    }
}

NewGameParameters::NewGameParameters(const NewGameParameters &src)
{
    skill_      = src.skill_;
    deathmatch_ = src.deathmatch_;

    map_ = src.map_;

    random_seed_   = src.random_seed_;
    total_players_ = src.total_players_;

    for (int i = 0; i < kMaximumPlayers; i++)
    {
        players_[i] = src.players_[i];
    }

    flags_ = nullptr;

    if (src.flags_)
        CopyFlags(src.flags_);
}

NewGameParameters::~NewGameParameters()
{
    if (flags_)
        delete flags_;
}

void NewGameParameters::SinglePlayer(int num_bots)
{
    total_players_ = 1 + num_bots;
    players_[0]    = kPlayerFlagNone; // i.e. !BOT and !NETWORK

    for (int pnum = 1; pnum <= num_bots; pnum++)
    {
        players_[pnum] = kPlayerFlagBot;
    }
}

void NewGameParameters::CopyFlags(const GameFlags *F)
{
    if (flags_)
        delete flags_;

    flags_ = new GameFlags;

    memcpy(flags_, F, sizeof(GameFlags));
}

//
// This is the procedure that changes the current_map
// at the start of the game and outside the normal
// progression of the game. All thats needed is the
// skill and the name (The name in the DDF File itself).
//
void DeferredNewGame(NewGameParameters &params)
{
    EPI_ASSERT(params.map_);

    defer_params = new NewGameParameters(params);

    if (params.level_skip_)
        defer_params->level_skip_ = true;

    game_action = kGameActionNewGame;
}

bool MapExists(const MapDefinition *map)
{
    return CheckPackFile(epi::StringFormat("%s.txt", map->name_.c_str()), "maps");
}

//
// REQUIRED STATE:
//   (a) defer_params
//
static void DoNewGame(void)
{
    EPI_ASSERT(defer_params);

    SaveClearSlot("current");

    InitNew(*defer_params);

    delete defer_params;
    defer_params = nullptr;    

    game_action = kGameActionLoadLevel;
}

//
// InitNew
//
// -ACB- 1998/07/12 Removed Lost Soul/Spectre Ability stuff
// -ACB- 1998/08/10 Inits new game without the need for gamemap or episode.
// -ACB- 1998/09/06 Removed remarked code.
// -KM- 1998/12/21 Added mapdef param so no need for defered init new
//   which was conflicting with net games.
//
// REQUIRED STATE:
//   ?? nothing ??
//
static void InitNew(NewGameParameters &params)
{
    // --- create players ---

    DestroyAllPlayers();

    for (int pnum = 0; pnum < kMaximumPlayers; pnum++)
    {
        if (params.players_[pnum] == kPlayerFlagNoPlayer)
            continue;

        CreatePlayer(pnum, (params.players_[pnum] & kPlayerFlagBot) ? true : false);

        if (console_player < 0 && !(params.players_[pnum] & kPlayerFlagBot) &&
            !(params.players_[pnum] & kPlayerFlagNetwork))
        {
            SetConsolePlayer(pnum);
        }
    }

    if (total_players != params.total_players_)
        FatalError("Internal Error: InitNew: player miscount (%d != %d)\n", total_players, params.total_players_);

    if (console_player < 0)
        FatalError("Internal Error: InitNew: no local players!\n");

    SetDisplayPlayer(console_player);

    if (paused)
    {
        paused = false;
        ResumeMusic(); // -ACB- 1999/10/07 New Music API
        ResumeSound();
    }

    current_map       = params.map_;
    current_hub_tag   = 0;
    current_hub_first = nullptr;

    if (params.skill_ > kSkillNightmare)
        params.skill_ = kSkillNightmare;

    RandomStateWrite(params.random_seed_);

    game_skill = params.skill_;
    deathmatch = params.deathmatch_;

    // LogDebug("GameInitNew: Deathmatch %d Skill %d\n", params.deathmatch,
    // (int)params.skill);

    // copy global flags into the level-specific flags
    if (params.flags_)
        level_flags = *params.flags_;
    else
        level_flags = global_flags;

    if (params.skill_ == kSkillNightmare)
    {
        level_flags.fast_monsters   = true;
        level_flags.enemies_respawn = true;
    }

    ResetTics();
}

void DeferredEndGame(void)
{
    if (game_state == kGameStateLevel)
    {
        game_action = kGameActionEndGame;
    }
}

//
// REQUIRED STATE:
//    ?? nothing ??
//
static void DoEndGame(void)
{
    DestroyAllPlayers();

    SaveClearSlot("current");

    if (game_state == kGameStateLevel)
    {
        BotEndLevel();

        // FIXME: LevelShutdownLevel()
    }

    game_state = kGameStateNothing;

    StopMusic();
}

bool CheckWhenAppear(AppearsFlag appear)
{
    if (!(appear & (1 << game_skill)))
        return false;

    if (InSinglePlayerMatch() && !(appear & kAppearsWhenSingle))
        return false;

    if (InCooperativeMatch() && !(appear & kAppearsWhenCoop))
        return false;

    if (InDeathmatch() && !(appear & kAppearsWhenDeathMatch))
        return false;

    return true;
}

MapDefinition *LookupMap(const char *refname)
{
    MapDefinition *m = mapdefs.Lookup(refname);

    if (m && MapExists(m))
        return m;

    // -AJA- handle numbers (like original DOOM)
    if (strlen(refname) <= 2 && epi::IsDigitASCII(refname[0]) && (!refname[1] || epi::IsDigitASCII(refname[1])))
    {
        int num = atoi(refname);
        // first try map names ending in ## (single digit treated as 0#)
        std::string map_check = epi::StringFormat("%02d", num);
        for (int i = mapdefs.size() - 1; i >= 0; i--)
        {
            if (mapdefs[i]->name_.size() >= 2)
            {
                if (epi::StringCaseCompareASCII(map_check, mapdefs[i]->name_.substr(mapdefs[i]->name_.size() - 2)) ==
                        0 &&
                    MapExists(mapdefs[i]) && mapdefs[i]->episode_)
                    return mapdefs[i];
            }
        }

        // otherwise try X#X# (episodic) style names
        if (1 <= num && num <= 9)
            num = num + 10;
        map_check = epi::StringFormat("E%dM%d", num / 10, num % 10);
        for (int i = mapdefs.size() - 1; i >= 0; i--)
        {
            if (mapdefs[i]->name_.size() == 4)
            {
                if (mapdefs[i]->name_[1] == map_check[1] && mapdefs[i]->name_[3] == map_check[3] &&
                    MapExists(mapdefs[i]) && mapdefs[i]->episode_)
                    return mapdefs[i];
            }
        }
    }

    return nullptr;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
