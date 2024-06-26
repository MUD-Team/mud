// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: c6c5b933ff38ff6efe6b6c49a86f0e10f6b862eb $
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
//	Moving object handling. Spawn functions.
//
//-----------------------------------------------------------------------------

#include "c_effect.h"
#include "g_gametype.h"
#include "g_spawninv.h"
#include "m_random.h"
#include "m_vectors.h"
#include "mud_includes.h"
#include "p_acs.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_common.h"
#include "s_sound.h"
#include "v_video.h"

EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(cl_showspawns)
EXTERN_CVAR(chasedemo)

void G_PlayerReborn(player_t &player);

//

//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//	between levels.
//
void P_SpawnPlayer(player_t &player, mapthing2_t *mthing)
{
    // denis - clients should not control spawning
    if (!serverside)
        return;

    // [RH] Things 4001-? are also multiplayer starts. Just like 1-4.
    //		To make things simpler, figure out which player is being
    //		spawned here.

    // not playing?
    if (!player.ingame())
        return;

    uint8_t playerstate = player.playerstate;

    if (player.doreborn)
    {
        G_PlayerReborn(player);
        player.doreborn = false;
    }

    AActor *mobj;
    //	if (player.deadspectator && player.mo)
    //		mobj = new AActor(player.mo->x, player.mo->y, ONFLOORZ, MT_PLAYER);
    //	else
    //		mobj = new AActor(mthing->x << FRACBITS, mthing->y << FRACBITS, ONFLOORZ, MT_PLAYER);
    mobj = new AActor(mthing->x << FRACBITS, mthing->y << FRACBITS, ONFLOORZ, MT_PLAYER);

    //	if (player.deadspectator && player.mo)
    //	{
    //		mobj->angle = player.mo->angle;
    //		mobj->pitch = player.mo->pitch;
    //	}
    //	else
    //	{
    //		mobj->angle = ANG45 * (mthing->angle/45);
    //		mobj->pitch = 0;
    //	}
    mobj->angle = ANG45 * (mthing->angle / 45);
    mobj->pitch = 0;

    mobj->player = &player;
    mobj->health = player.health;

    player.fov = 90.0f;
    player.mo = player.camera = mobj->ptr();
    player.playerstate        = PST_LIVE;
    player.refire             = 0;
    player.damagecount        = 0;
    player.bonuscount         = 0;
    player.extralight         = 0;
    player.fixedcolormap      = 0;
    player.viewheight         = VIEWHEIGHT;
    player.xviewshift         = 0;
    player.attacker           = AActor::AActorPtr();

    consoleplayer().camera = displayplayer().mo;

    // Set up some special spectator stuff
    if (player.spectator)
    {
        mobj->translucency = 0;
        player.mo->oflags |= MFO_SPECTATOR;
        player.mo->flags2 |= MF2_FLY;
        player.mo->flags &= ~MF_SOLID;
    }

    // setup gun psprite
    P_SetupPsprites(&player);

    // give all cards in death match mode
    if (!G_IsCoopGame())
    {
        for (int32_t i = 0; i < NUMCARDS; i++)
            player.cards[i] = true;
    }

    // Give any other between-level inventory.
    if (!player.spectator)
        G_GiveBetweenInventory(player);

    // [RH] If someone is in the way, kill them
    P_TeleportMove(mobj, mobj->x, mobj->y, mobj->z, true);

    // [BC] Do script stuff
    if (serverside && level.behavior)
    {
        if (playerstate == PST_ENTER)
            level.behavior->StartTypedScripts(SCRIPT_Enter, player.mo);
        else if (playerstate == PST_REBORN)
            level.behavior->StartTypedScripts(SCRIPT_Respawn, player.mo);
    }
}

std::vector<AActor *> spawnfountains;

/**
 * Show spawn points as particle fountains
 * ToDo: Make an independant spawning loop to handle these.
 */
void P_ShowSpawns(mapthing2_t *mthing)
{
    // Ch0wW: DO NOT add new spawns to a DOOM2 demo !
    // It'll immediately desync in DM!

    if (clientside && cl_showspawns)
    {
        AActor *spawn = 0;

        if (sv_gametype == GM_DM && mthing->type == 11)
        {
            // [RK] If we're not using z-height spawns, spawn the fountain on the floor
            spawn = new AActor(mthing->x << FRACBITS, mthing->y << FRACBITS,
                               (level.flags & LEVEL_USEPLAYERSTARTZ ? mthing->z << FRACBITS : ONFLOORZ), MT_FOUNTAIN);

            spawn->args[0] = 7; // White
        }

        if (G_IsTeamGame())
        {
            for (int32_t iTeam = 0; iTeam < NUMTEAMS; iTeam++)
            {
                TeamInfo *teamInfo = GetTeamInfo((team_t)iTeam);
                if (teamInfo->TeamSpawnThingNum == mthing->type)
                {
                    // [RK] If we're not using z-height spawns, spawn the fountain on the floor
                    spawn = new AActor(mthing->x << FRACBITS, mthing->y << FRACBITS,
                                       (level.flags & LEVEL_USEPLAYERSTARTZ ? mthing->z << FRACBITS : ONFLOORZ),
                                       MT_FOUNTAIN);

                    spawn->args[0] = teamInfo->FountainColorArg;
                    break;
                }
            }
        }

        if (spawn)
        {
            spawn->effects = spawn->args[0] << FX_FOUNTAINSHIFT;
        }
    }
}

VERSION_CONTROL(cl_mobj_cpp, "$Id: c6c5b933ff38ff6efe6b6c49a86f0e10f6b862eb $")
