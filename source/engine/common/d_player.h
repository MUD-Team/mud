// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 7240e95cb85a132cc747ffcc286d4d69f0cf0087 $
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
//	D_PLAYER
//
//-----------------------------------------------------------------------------

#pragma once

#include <time.h>

#include <list>
#include <queue>

#include "actor.h"
#include "d_items.h"
#include "d_net.h"
#include "d_netcmd.h"
#include "d_netinf.h"
#include "d_ticcmd.h"
#include "doomtype.h"
#include "huffman.h"
#include "i_net.h"
#include "p_pspr.h"
#include "p_snapshot.h"

//
// Player states.
//
typedef enum
{
    // Connecting or hacking
    PST_CONTACT,

    // Stealing or pirating
    PST_DOWNLOAD,

    // Staling or loitering
    PST_SPECTATE,

    // Spying or remote server administration
    PST_STEALTH_SPECTATE,

    // Playing or camping.
    PST_LIVE,

    // Dead on the ground, view follows killer.
    PST_DEAD,

    // Ready to restart/respawn???
    PST_REBORN,

    // These are cleaned up at the end of a frame
    PST_DISCONNECT,

    // [BC] Entered the game
    PST_ENTER

} playerstate_t;

//
// Player internal flags, for cheats and debug.
//
typedef enum
{
    CF_NOCLIP       = (1 << 0), // No clipping, walk through barriers.
    CF_GODMODE      = (1 << 1), // No damage, no health loss.
    CF_NOMOMENTUM   = (1 << 2), // Not really a cheat, just a debug aid.
    CF_NOTARGET     = (1 << 3), // [RH] Monsters don't target
    CF_FLY          = (1 << 4), // [RH] Flying player
    CF_CHASECAM     = (1 << 5), // [RH] Put camera behind player
    CF_FROZEN       = (1 << 6), // [RH] Don't let the player move
    CF_REVERTPLEASE = (1 << 7), // [RH] Stick camera in player's head if he moves
    CF_BUDDHA       = (1 << 8), // [Ch0wW] Buddha Cheatcode
} cheat_t;

#define MAX_PLAYER_SEE_MOBJ 0x7F

static const int32_t ReJoinDelay  = TICRATE * 5;
static const int32_t SuicideDelay = TICRATE * 10;

//
// Extended player object info: player_t
//
class player_s
{
  public:
    void Serialize(FArchive &arc);

    bool ingame() const
    {
        return playerstate == PST_LIVE || playerstate == PST_DEAD || playerstate == PST_REBORN ||
               playerstate == PST_ENTER;
    }

    // player identifier on server
    uint8_t id;

    // current player state, see playerstate_t
    uint8_t playerstate;

    AActor::AActorPtr mo;

    struct ticcmd_t        cmd;      // the ticcmd currently being processed
    std::queue<NetCommand> cmdqueue; // all received ticcmds

    // [RH] who is this?
    UserInfo userinfo;

    // FOV in degrees
    float fov;
    // Focal origin above r.z
    fixed_t viewz;
    fixed_t prevviewz;
    // Base height above floor for viewz.
    fixed_t viewheight;
    // Bob/squat speed.
    fixed_t deltaviewheight;
    // bounded/scaled total momentum.
    fixed_t bob;

    // This is only used between levels,
    // mo->health is used during levels.
    int32_t health;
    int32_t armorpoints;
    // Armor type is 0-2.
    int32_t armortype;

    // Power ups. invinc and invis are tic counters.
    int32_t powers[NUMPOWERS];
    bool    cards[NUMCARDS];
    bool    backpack;

    // [AM] Lives left.
    int32_t lives;
    // [AM] Rounds won in round-based games.
    int32_t roundwins;
    // [Toke - CTF] Points in a special game mode
    int32_t points;
    // [Toke - CTF - Carry] Remembers the flag when grabbed
    bool flags[NUMTEAMS];

    // Frags, deaths, monster kills
    int32_t fragcount;
    int32_t deathcount;
    int32_t monsterdmgcount;
    int32_t killcount, itemcount, secretcount; // for intermission

    // Total points/frags that aren't reset after rounds. Used for LMS/TLMS/LMSCTF.
    int32_t totalpoints;
    // Total Deaths that are seen only on Rounds without lives.
    int32_t totaldeaths;

    // Is wp_nochange if not changing.
    weapontype_t pendingweapon;
    weapontype_t readyweapon;

    bool    weaponowned[NUMWEAPONS + 1];
    int32_t ammo[NUMAMMO];
    int32_t maxammo[NUMAMMO];

    // True if button down last tic.
    int32_t attackdown, usedown;

    // Bit flags, for cheats and debug.
    // See cheat_t, above.
    uint32_t cheats;

    // Refired shots are less accurate.
    int16_t refire;

    // For screen flashing (red or bright).
    int32_t damagecount, bonuscount;

    // Who did damage (NULL for floors/ceilings).
    AActor::AActorPtrCounted attacker;

    // So gun flashes light up areas.
    int32_t extralight;
    // Current PLAYPAL, ???
    int32_t fixedcolormap;           //  can be set to REDCOLORMAP for pain, etc.

    int32_t xviewshift;              // [RH] view shift (for earthquakes)

    int32_t  psprnum;
    pspdef_t psprites[NUMPSPRITES];  // Overlay view sprites (gun, etc).

    int32_t jumpTics;                // delay the next jump for a moment

    int32_t death_time;              // [SL] Record time of death to enforce respawn delay if needed
    int32_t suicidedelay;            // Ch0wW - Time between 2 suicides.
    fixed_t oldvelocity[3];          // [RH] Used for falling damage

    AActor::AActorPtr camera;        // [RH] Whose eyes this player sees through

    int32_t air_finished;            // [RH] Time when you start drowning

    int32_t GameTime;                // [Dash|RD] Length of time that this client has been in the game.
    time_t  JoinTime;                // [Dash|RD] Time this client joined.
    int32_t ping;                    // [Fly] guess what :)
    int32_t last_received;

    int32_t tic;                     // gametic last update for player was received

    PlayerSnapshotManager snapshots; // Previous player positions

    uint8_t spying;                  // [SL] id of player being spynext'd by this player
    bool    spectator;               // [GhostlyDeath] spectating?
                                     //	bool		deadspectator;			// [tm512] spectating as a dead player?
    int32_t joindelay;               // Number of tics to delay player from rejoining
    int32_t timeout_callvote;        // [AM] Tic when a vote last finished.
    int32_t timeout_vote;            // [AM] Tic when a player last voted.

    bool    ready;                   // [AM] Player is ready.
    int32_t timeout_ready;           // [AM] Tic when a player last toggled his ready state.

    uint8_t prefcolor[4];            // Nes - Preferred color. Server only.

    argb_t blend_color;              // blend color for the sector the player is in
    bool   doreborn;

    uint8_t QueuePosition;           // Queue position to join game. 0 means not in queue

    // zdoom
    int32_t hazardcount;
    uint8_t hazardinterval;

    // For flood protection
    struct LastMessage_s
    {
        dtime_t     Time;
        std::string Message;
    } LastMessage;

    // denis - things that are pending to be sent to this player
    std::queue<AActor::AActorPtr> to_spawn;

    // denis - client structure is here now for a 1:1
    struct client_t
    {
        struct oldPacket_t
        {
            int32_t sequence;
            buf_t   data;

            oldPacket_t() : sequence(-1)
            {
                data.resize(0);
            }

            oldPacket_t(const oldPacket_t &other)
            {
                sequence = other.sequence;
                data     = other.data;
            }
        };

        netadr_t address;

        buf_t netbuf;
        buf_t reliablebuf;

        // protocol version supported by the client
        int16_t version;
        int32_t packedversion;

        // for reliable protocol
        oldPacket_t oldpackets[256];

        int32_t sequence;
        int32_t last_sequence;
        uint8_t packetnum;

        int32_t rate;
        int32_t reliable_bps;  // bytes per second
        int32_t unreliable_bps;

        int32_t last_received; // for timeouts

        int32_t lastcmdtic, lastclientcmdtic;

        std::string digest;     // randomly generated string that the client must use for any hashes it sends back
        bool        allow_rcon; // allow remote admin
        bool        displaydisconnect; // display disconnect message when disconnecting

        huffman_server compressor;     // denis - adaptive huffman compression

        class download_t
        {
          public:
            std::string name;
            std::string md5;
            uint32_t    next_offset;

            download_t() : name(""), md5(""), next_offset(0)
            {
            }
            download_t(const download_t &other) : name(other.name), md5(other.md5), next_offset(other.next_offset)
            {
            }
        } download;

        client_t()
        {
            // GhostlyDeath -- Initialize to Zero
            memset(&address, 0, sizeof(netadr_t));
            version       = 0;
            packedversion = 0;
            for (size_t i = 0; i < ARRAY_LENGTH(oldpackets); i++)
            {
                oldpackets[i].sequence = -1;
                oldpackets[i].data.resize(MAX_UDP_PACKET);
            }
            sequence         = 0;
            last_sequence    = 0;
            packetnum        = 0;
            rate             = 0;
            reliable_bps     = 0;
            unreliable_bps   = 0;
            last_received    = 0;
            lastcmdtic       = 0;
            lastclientcmdtic = 0;

            // GhostlyDeath -- done with the {}
            netbuf            = MAX_UDP_PACKET;
            reliablebuf       = MAX_UDP_PACKET;
            digest            = "";
            allow_rcon        = false;
            displaydisconnect = true;
            /*
            huffman_server	compressor;	// denis - adaptive huffman compression*/
        }
        client_t(const client_t &other)
            : address(other.address), netbuf(other.netbuf), reliablebuf(other.reliablebuf), version(other.version),
              packedversion(other.packedversion), sequence(other.sequence), last_sequence(other.last_sequence),
              packetnum(other.packetnum), rate(other.rate), reliable_bps(other.reliable_bps),
              unreliable_bps(other.unreliable_bps), last_received(other.last_received), lastcmdtic(other.lastcmdtic),
              lastclientcmdtic(other.lastclientcmdtic), digest(other.digest), allow_rcon(false),
              displaydisconnect(true), compressor(other.compressor), download(other.download)
        {
            for (size_t i = 0; i < ARRAY_LENGTH(oldpackets); i++)
            {
                oldpackets[i] = other.oldpackets[i];
            }
        }
    } client;

    struct ticcmd_t netcmds[BACKUPTICS];

    int32_t GetPlayerNumber() const
    {
        return id - 1;
    }

    player_s();
    player_s &operator=(const player_s &other);

    ~player_s();
};

typedef player_s           player_t;
typedef player_t::client_t client_t;

// Bookkeeping on players - state.
typedef std::list<player_t> Players;
extern Players              players;

// Player taking events, and displaying.
player_t &consoleplayer();
player_t &displayplayer();
player_t &listenplayer();
player_t &idplayer(uint8_t id);
player_t &nameplayer(const std::string &netname);
bool      validplayer(player_t &ref);

/**
 * @brief A collection of pointers to players, commonly called a "view".
 */
typedef std::vector<player_t *> PlayersView;

/**
 * @brief Results of a PlayerQuery.
 */
struct PlayerResults
{
    /**
     * @brief Number of results returned.
     */
    int32_t count;

    /**
     * @brief Number of results returned per team.
     */
    int32_t teamCount[NUMTEAMS];

    /**
     * @brief Total number of players scanned.
     */
    int32_t total;

    /**
     * @brief Total number of players per team.
     */
    int32_t teamTotal[NUMTEAMS];

    /**
     * @brief A view containing player pointers that satisfy the query.
     */
    PlayersView players;

    PlayerResults() : count(0), total(0)
    {
        for (size_t i = 0; i < ARRAY_LENGTH(teamCount); i++)
            teamCount[i] = 0;
        for (size_t i = 0; i < ARRAY_LENGTH(teamTotal); i++)
            teamTotal[i] = 0;
    }
};

class PlayerQuery
{
    enum SortTypes
    {
        SORT_NONE,
        SORT_FRAGS,
        SORT_LIVES,
        SORT_WINS,
    };

    enum SortFilters
    {
        SFILTER_NONE,
        SFILTER_MAX,
        SFILTER_NOT_MAX,
    };

    bool        m_ready;
    bool        m_health;
    bool        m_lives;
    bool        m_notLives;
    team_t      m_team;
    SortTypes   m_sort;
    SortFilters m_sortFilter;

  public:
    PlayerQuery()
        : m_ready(false), m_health(false), m_lives(false), m_notLives(false), m_team(TEAM_NONE), m_sort(SORT_NONE),
          m_sortFilter(SFILTER_NONE)
    {
    }

    /**
     * @brief Check for ready players only.
     *
     * @return A mutated PlayerQuery to chain off of.
     */
    PlayerQuery &isReady()
    {
        m_ready = true;
        return *this;
    }

    /**
     * @brief Check for players with health left.
     *
     * @return A mutated PlayerQuery to chain off of.
     */
    PlayerQuery &hasHealth()
    {
        m_health = true;
        return *this;
    }

    /**
     * @brief Check for players with lives left.
     *
     * @return A mutated PlayerQuery to chain off of.
     */
    PlayerQuery &hasLives()
    {
        m_lives = true;
        return *this;
    }

    /**
     * @brief Check for players with no lives left.
     *
     * @return A mutated PlayerQuery to chain off of.
     */
    PlayerQuery &notHasLives()
    {
        m_notLives = true;
        return *this;
    }

    /**
     * @brief Filter players by a specific team.
     *
     * @param team Team to filter by.
     * @return A mutated PlayerQuery to chain off of.
     */
    PlayerQuery &onTeam(team_t team)
    {
        m_team = team;
        return *this;
    }

    /**
     * @brief Return players with the top frag count, whatever that may be.
     *
     * @return A mutated PlayerQuery to chain off of.
     */
    PlayerQuery &sortFrags()
    {
        m_sort = SORT_FRAGS;
        return *this;
    }

    /**
     * @brief Return players with the top lives count, whomever that may be.
     *
     * @return A mutated PlayerQuery to chain off of.
     */
    PlayerQuery &sortLives()
    {
        m_sort = SORT_LIVES;
        return *this;
    }

    /**
     * @brief Return players with the top win count, whatever that may be.
     *
     * @return A mutated PlayerQuery to chain off of.
     */
    PlayerQuery &sortWins()
    {
        m_sort = SORT_WINS;
        return *this;
    }

    /**
     * @brief Given a sort, filter so only the top item remains.  In the case
     *        of a tie, multiple items are returned.
     *
     * @return A mutated PlayerQuery to chain off of.
     */
    PlayerQuery &filterSortMax()
    {
        m_sortFilter = SFILTER_MAX;
        return *this;
    }

    /**
     * @brief Given a sort, filter so only things other than the top item
     *        remains.
     *
     * @return A mutated PlayerQuery to chain off of.
     */
    PlayerQuery &filterSortNotMax()
    {
        m_sortFilter = SFILTER_NOT_MAX;
        return *this;
    }

    PlayerResults execute();
};

class SpecQuery
{
    bool m_onlyInQueue;

  public:
    SpecQuery() : m_onlyInQueue(false)
    {
    }

    /**
     * @brief Filter out players who are not in the queue.
     *
     * @return A mutated SpecQuery to chain off of.
     */
    SpecQuery &onlyInQueue()
    {
        m_onlyInQueue = true;
        return *this;
    }

    PlayersView execute();
};

enum
{
    SCORES_CLEAR_WINS        = (1 << 0),
    SCORES_CLEAR_POINTS      = (1 << 1),
    SCORES_CLEAR_TOTALPOINTS = (1 << 2),
    SCORES_CLEAR_ALL         = (0xFF),
};

void   P_ClearPlayerCards(player_t &p);
void   P_ClearPlayerPowerups(player_t &p);
void   P_ClearPlayerScores(player_t &p, uint8_t flags);
size_t P_NumPlayersInGame();
size_t P_NumReadyPlayersInGame();
size_t P_NumPlayersOnTeam(team_t team);

extern uint8_t consoleplayer_id;
extern uint8_t displayplayer_id;

//

// INTERMISSION
// Structure passed e.g. to WI_Start(wb)
//
typedef struct wbplayerstruct_s
{
    bool in; // whether the player is in game

    // Player stats, kills, collected items etc.
    int32_t skills;
    int32_t sitems;
    int32_t ssecret;
    int32_t stime;
    int32_t fragcount; // [RH] Cumulative frags for this player
    int32_t score;     // current score on entry, modified on return

} wbplayerstruct_t;

typedef struct wbstartstruct_s
{
    int32_t epsd;    // episode # (0-2)

    char current[9]; // [RH] Name of map just finished
    char next[9];    // next level, [RH] actual map name

    char lname0[9];
    char lname1[9];

    int32_t maxkills;
    int32_t maxitems;
    int32_t maxsecret;
    int32_t maxfrags;

    // the par time
    int32_t partime;

    // index of this player in game
    uint32_t pnum;

    std::vector<wbplayerstruct_s> plyr;
} wbstartstruct_t;
