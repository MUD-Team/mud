// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 4f832363d58078d4532a94eb5615de229bf8fac6 $
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
//  Old version of the server query protocol, kept for clients and older
//  launchers
//
//-----------------------------------------------------------------------------

#include "d_main.h"
#include "d_player.h"
#include "g_gametype.h"
#include "i_system.h"
#include "mud_includes.h"

static buf_t ml_message(MAX_UDP_PACKET);

EXTERN_CVAR(sv_scorelimit) // [CG] Should this go below in //bond ?

EXTERN_CVAR(sv_usemasters)
EXTERN_CVAR(sv_hostname)
EXTERN_CVAR(sv_maxclients)

EXTERN_CVAR(port)

// bond===========================
EXTERN_CVAR(sv_timelimit)
EXTERN_CVAR(sv_fraglimit)
EXTERN_CVAR(sv_email)
EXTERN_CVAR(sv_itemsrespawn)
EXTERN_CVAR(sv_weaponstay)
EXTERN_CVAR(sv_friendlyfire)
EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR(sv_infiniteammo)
EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_monstersrespawn)
EXTERN_CVAR(sv_fastmonsters)
EXTERN_CVAR(sv_waddownload)
EXTERN_CVAR(sv_emptyreset)
EXTERN_CVAR(sv_fragexitswitch)
// bond===========================

EXTERN_CVAR(sv_teamsinplay)

EXTERN_CVAR(sv_maxplayers)
EXTERN_CVAR(join_password)
EXTERN_CVAR(sv_downloadsites)

EXTERN_CVAR(sv_natport)

//
// denis - each launcher reply contains a random token so that
// the server will only allow connections with a valid token
// in order to protect itself from ip spoofing
//
struct token_t
{
    uint32_t id;
    uint64_t issued;
    netadr_t from;
};

#define MAX_TOKEN_AGE (20 * TICRATE) // 20s should be enough for any client to load its wads
static std::vector<token_t> connect_tokens;

//
// SV_NewToken
//
uint32_t SV_NewToken()
{
    uint64_t now = I_MSTime() * TICRATE / 1000;

    token_t token;
    token.id     = rand() * time(0);
    token.issued = now;
    token.from   = net_from;

    // find an old token to replace
    for (size_t i = 0; i < connect_tokens.size(); i++)
    {
        if (now - connect_tokens[i].issued >= MAX_TOKEN_AGE)
        {
            connect_tokens[i] = token;
            return token.id;
        }
    }

    connect_tokens.push_back(token);

    return token.id;
}

//
// SV_ValidToken
//
bool SV_IsValidToken(uint32_t token)
{
    uint64_t now = I_MSTime() * TICRATE / 1000;

    for (size_t i = 0; i < connect_tokens.size(); i++)
    {
        if (connect_tokens[i].id == token && NET_CompareAdr(connect_tokens[i].from, net_from) &&
            now - connect_tokens[i].issued < MAX_TOKEN_AGE)
        {
            // extend token life and confirm
            connect_tokens[i].issued = now;
            return true;
        }
    }

    return false;
}

//
// SV_SendServerInfo
//
// Sends server info to a launcher
// TODO: Clean up and reinvent.
void SV_SendServerInfo()
{
    size_t i;

    SZ_Clear(&ml_message);

    MSG_WriteLong(&ml_message, MSG_CHALLENGE);
    MSG_WriteLong(&ml_message, SV_NewToken());

    // if master wants a key to be presented, present it we will
    if (MSG_BytesLeft() == 4)
        MSG_WriteLong(&ml_message, MSG_ReadLong());

    MSG_WriteString(&ml_message, (char *)sv_hostname.cstring());

    uint8_t playersingame = 0;
    for (Players::iterator it = players.begin(); it != players.end(); ++it)
    {
        if (it->ingame())
            playersingame++;
    }

    MSG_WriteByte(&ml_message, playersingame);
    MSG_WriteByte(&ml_message, sv_maxclients.asInt());

    MSG_WriteString(&ml_message, level.mapname.c_str());

    size_t numwads = wadfiles.size();
    if (numwads > 0xff)
        numwads = 0xff;

    MSG_WriteByte(&ml_message, numwads);

    for (i = 0; i < numwads; ++i)
        MSG_WriteString(&ml_message, wadfiles[i].getBasename().c_str());

    MSG_WriteBool(&ml_message, (sv_gametype == GM_DM || sv_gametype == GM_TEAMDM));
    MSG_WriteByte(&ml_message, sv_skill.asInt());
    MSG_WriteBool(&ml_message, (sv_gametype == GM_TEAMDM));

    for (Players::iterator it = players.begin(); it != players.end(); ++it)
    {
        if (it->ingame())
        {
            MSG_WriteString(&ml_message, it->userinfo.netname.c_str());
            MSG_WriteShort(&ml_message, it->fragcount);
            MSG_WriteLong(&ml_message, it->ping);

            if (G_IsTeamGame())
                MSG_WriteByte(&ml_message, it->userinfo.team);
            else
                MSG_WriteByte(&ml_message, TEAM_NONE);
        }
    }

    for (i = 1; i < numwads; ++i)
        MSG_WriteString(&ml_message, ::wadfiles[i].getMD5().getHexCStr());

    // [AM] Used to be sv_website - sv_downloadsites can have multiple sites.
    MSG_WriteString(&ml_message, sv_downloadsites.cstring());

    if (G_IsTeamGame())
    {
        MSG_WriteLong(&ml_message, sv_scorelimit.asInt());

        for (size_t i = 0; i < NUMTEAMS; i++)
        {
            MSG_WriteByte(&ml_message, 0);
        }
    }

    MSG_WriteShort(&ml_message, VERSION);

    // bond===========================
    MSG_WriteString(&ml_message, (char *)sv_email.cstring());

    int32_t timeleft = (int32_t)(sv_timelimit - level.time / (TICRATE * 60));
    if (timeleft < 0)
        timeleft = 0;

    MSG_WriteShort(&ml_message, sv_timelimit.asInt());
    MSG_WriteShort(&ml_message, timeleft);
    MSG_WriteShort(&ml_message, sv_fraglimit.asInt());

    MSG_WriteBool(&ml_message, (sv_itemsrespawn ? true : false));
    MSG_WriteBool(&ml_message, (sv_weaponstay ? true : false));
    MSG_WriteBool(&ml_message, (sv_friendlyfire ? true : false));
    MSG_WriteBool(&ml_message, (sv_allowexit ? true : false));
    MSG_WriteBool(&ml_message, (sv_infiniteammo ? true : false));
    MSG_WriteBool(&ml_message, (sv_nomonsters ? true : false));
    MSG_WriteBool(&ml_message, (sv_monstersrespawn ? true : false));
    MSG_WriteBool(&ml_message, (sv_fastmonsters ? true : false));
    MSG_WriteBool(&ml_message, (sv_waddownload ? true : false));
    MSG_WriteBool(&ml_message, (sv_emptyreset ? true : false));
    MSG_WriteBool(&ml_message, false); // used to be sv_cleanmaps
    MSG_WriteBool(&ml_message, (sv_fragexitswitch ? true : false));

    for (Players::iterator it = players.begin(); it != players.end(); ++it)
    {
        if (it->ingame())
        {
            MSG_WriteShort(&ml_message, it->killcount);
            MSG_WriteShort(&ml_message, it->deathcount);

            int32_t timeingame = (time(NULL) - it->JoinTime) / 60;
            if (timeingame < 0)
                timeingame = 0;
            MSG_WriteShort(&ml_message, timeingame);
        }
    }

    // bond===========================

    MSG_WriteLong(&ml_message, (uint32_t)0x01020304);
    MSG_WriteShort(&ml_message, sv_maxplayers.asInt());

    for (Players::iterator it = players.begin(); it != players.end(); ++it)
    {
        if (it->ingame())
        {
            MSG_WriteBool(&ml_message, (it->spectator ? true : false));
        }
    }

    MSG_WriteLong(&ml_message, (uint32_t)0x01020305);
    MSG_WriteShort(&ml_message, strlen(join_password.cstring()) ? 1 : 0);

    // GhostlyDeath -- Send Game Version info
    MSG_WriteLong(&ml_message, GAMEVER);

    NET_SendPacket(ml_message, net_from);
}

VERSION_CONTROL(sv_sqpold_cpp, "$Id: 4f832363d58078d4532a94eb5615de229bf8fac6 $")
