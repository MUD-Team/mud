// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: cdadbbdf27ddca817b5e38bdece562a94dd5ebe8 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Handlers for messages sent from the server.
//
//-----------------------------------------------------------------------------

#include "cl_parse.h"

#include <bitset>

#include "c_console.h"
#include "c_dispatch.h"
#include "c_effect.h"
#include "c_maplist.h"
#include "cl_main.h"
#include "cl_maplist.h"
#include "cl_replay.h"
#include "cl_vote.h"
#include "cmdlib.h"
#include "d_main.h"
#include "d_player.h"
#include "g_gametype.h"
#include "g_levelstate.h"
#include "gi.h"
#include "i_video.h"
#include "infomap.h"
#include "m_argv.h"
#include "m_random.h"
#include "m_resfile.h"
#include "m_strindex.h"
#include "mud_includes.h"
#include "p_acs.h"
#include "p_inter.h"
#include "p_lnspec.h"
#include "p_mapformat.h"
#include "p_mobj.h"
#include "r_client.h"
#include "r_common.h"
#include "r_sky.h"
#include "res_texture.h"
#include "s_sound.h"
#include "server.pb.h"
#include "svc_map.h"
#include "v_palette.h"
#include "v_textcolors.h"

// Extern data from other files.

EXTERN_CVAR(cl_autorecord)
EXTERN_CVAR(cl_autorecord_coop)
EXTERN_CVAR(cl_autorecord_deathmatch)
EXTERN_CVAR(cl_autorecord_duel)
EXTERN_CVAR(cl_autorecord_teamdm)
EXTERN_CVAR(cl_chatsounds)
EXTERN_CVAR(cl_connectalert)
EXTERN_CVAR(cl_disconnectalert)
EXTERN_CVAR(cl_team)
EXTERN_CVAR(hud_revealsecrets)
EXTERN_CVAR(mute_enemies)
EXTERN_CVAR(mute_spectators)
EXTERN_CVAR(show_messages)

extern std::string                               digest;
extern int32_t                                   last_svgametic;
extern int32_t                                   last_player_update;
extern NetCommand                                localcmds[MAXSAVETICS];
extern bool                                      recv_full_update;
extern std::map<uint16_t, SectorSnapshotManager> sector_snaps;
extern std::set<uint8_t>                         teleported_players;

void      CL_CheckDisplayPlayer(void);
void      CL_ClearPlayerJustTeleported(player_t *player);
void      CL_ClearSectorSnapshots();
player_t &CL_FindPlayer(size_t id);
bool      CL_PlayerJustTeleported(player_t *player);
void      CL_QuitAndTryDownload(const OWantFile &missing_file);
void      CL_ResyncWorldIndex();
void      CL_SpectatePlayer(player_t &player, bool spectate);
void      G_PlayerReborn(player_t &p); // [Toke - todo] clean this function
void      P_DestroyButtonThinkers();
void      P_ExplodeMissile(AActor *mo);
void      P_PlayerLeavesGame(player_s *player);
void      P_SetPsprite(player_t *player, int32_t position, statenum_t stnum);
void      P_SetButtonTexture(line_t *line, texhandle_t texture);

/**
 * @brief Unpack a bitfield into an array of booleans.
 */
static void UnpackBoolArray(bool *bools, size_t count, uint32_t in)
{
    for (size_t i = 0; i < count; i++)
    {
        bools[i] = in & BIT(i);
    }
}

/**
 * @brief Common code for activating a line.
 */
static void ActivateLine(AActor *mo, line_s *line, uint8_t side, LineActivationType activationType,
                         const bool bossaction, uint8_t special = 0, int32_t arg0 = 0, int32_t arg1 = 0,
                         int32_t arg2 = 0, int32_t arg3 = 0, int32_t arg4 = 0)
{
    // [SL] 2012-03-07 - If this is a player teleporting, add this player to
    // the set of recently teleported players.  This is used to flush past
    // positions since they cannot be used for interpolation.
    if (line && (mo && mo->player) && (P_IsTeleportLine(line->special)))
    {
        teleported_players.insert(mo->player->id);

        // [SL] 2012-03-21 - Server takes care of moving players that teleport.
        // Don't allow client to process it since it screws up interpolation.
        return;
    }

    // [SL] 2012-04-25 - Clients will receive updates for sectors so they do not
    // need to create moving sectors on their own in response to svc_activateline
    if (line && P_LineSpecialMovesSector(line->special))
        return;

    s_SpecialFromServer = true;

    switch (activationType)
    {
    case LineCross:
        if (line)
            P_CrossSpecialLine(line, side, mo, bossaction);
        break;
    case LineUse:
        if (line)
            P_UseSpecialLine(mo, line, side, bossaction);
        break;
    case LineShoot:
        if (line)
            P_ShootSpecialLine(mo, line);
        break;
    case LinePush:
        if (line)
            P_PushSpecialLine(mo, line, side);
        break;
    case LineACS:
        LineSpecials[special](line, mo, arg0, arg1, arg2, arg3, arg4);
        break;
    default:
        break;
    }

    s_SpecialFromServer = false;
}

/**
 * @brief svc_noop - Nothing to see here. Move along.
 */
static void CL_Noop(const odaproto::svc::Noop *msg)
{
}

/**
 * @brief svc_disconnect - Disconnect a client from the server.
 */
static void CL_Disconnect(const odaproto::svc::Disconnect *msg)
{
    std::string buffer;
    if (!msg->message().empty())
    {
        StrFormat(buffer, "Disconnected from server: %s", msg->message().c_str());
    }
    else
    {
        StrFormat(buffer, "Disconnected from server\n");
    }

    Printf("%s", msg->message().c_str());
    CL_QuitNetGame(NQ_SILENT);
}

/**
 * @brief svc_playerinfo - Your personal arsenal, as supplied by the server.
 */
static void CL_PlayerInfo(const odaproto::svc::PlayerInfo *msg)
{
    player_t &p = consoleplayer();

    uint32_t weaponowned = msg->player().weaponowned();
    UnpackBoolArray(p.weaponowned, NUMWEAPONS, weaponowned);

    uint32_t cards = msg->player().cards();
    UnpackBoolArray(p.cards, NUMCARDS, cards);

    p.backpack = msg->player().backpack();

    for (int32_t i = 0; i < NUMAMMO; i++)
    {
        if (i < msg->player().ammo_size())
            p.ammo[i] = msg->player().ammo(i);
        else
            p.ammo[i] = 0;

        if (i < msg->player().maxammo_size())
            p.maxammo[i] = msg->player().maxammo(i);
        else
            p.maxammo[i] = 0;
    }

    p.health      = msg->player().health();
    p.armorpoints = msg->player().armorpoints();
    p.armortype   = msg->player().armortype();

    if (p.lives == 0 && msg->player().lives() > 0)
    {
        // Stop spying so you know you're back from the dead.
        ::displayplayer_id = ::consoleplayer_id;
    }
    p.lives = msg->player().lives();

    weapontype_t pending = static_cast<weapontype_t>(msg->player().pendingweapon());
    if (pending != wp_nochange && pending < NUMWEAPONS)
    {
        p.pendingweapon = pending;
    }
    weapontype_t readyweapon = static_cast<weapontype_t>(msg->player().readyweapon());
    if (readyweapon != p.readyweapon && readyweapon < NUMWEAPONS)
    {
        p.pendingweapon = readyweapon;
    }

    // Tic was replayed? Don't try and use the replays's autoswitch at the same tic as weapon correction.
    if (ClientReplay::getInstance().wasReplayed() && pending == wp_nochange)
    {
        p.pendingweapon = wp_nochange;
    }

    for (int32_t i = 0; i < NUMPOWERS; i++)
    {
        if (i < msg->player().powers_size())
        {
            p.powers[i] = msg->player().powers(i);
        }
        else
        {
            p.powers[i] = 0;
        }
    }

    if (!p.spectator)
        p.cheats = msg->player().cheats();

    // If a full update was declared, don't try and correct any weapons.
    ClientReplay::getInstance().reset();
}

/**
 * @brief svc_moveplayer - Move a player.
 */
static void CL_MovePlayer(const odaproto::svc::MovePlayer *msg)
{
    uint8_t   who = msg->player().playerid();
    player_t *p   = &idplayer(who);

    fixed_t x = msg->actor().pos().x();
    fixed_t y = msg->actor().pos().y();
    fixed_t z = msg->actor().pos().z();

    angle_t angle = msg->actor().angle();
    angle_t pitch = msg->actor().pitch();

    int32_t frame = msg->frame();
    fixed_t momx  = msg->actor().mom().x();
    fixed_t momy  = msg->actor().mom().y();
    fixed_t momz  = msg->actor().mom().z();

    int32_t invisibility = 0;
    if (msg->player().powers_size() >= pw_invisibility)
        invisibility = msg->player().powers().Get(pw_invisibility);

    if (!validplayer(*p) || !p->mo)
        return;

    // Mark the gametic this update arrived in for prediction code
    p->tic = gametic;

    // GhostlyDeath -- Servers will never send updates on spectators
    if (p->spectator && (p != &consoleplayer()))
        p->spectator = 0;

    // [Russell] - hack, read and set invisibility flag
    p->powers[pw_invisibility] = invisibility;
    if (p->powers[pw_invisibility])
        p->mo->flags |= MF_SHADOW;
    else
        p->mo->flags &= ~MF_SHADOW;

    // This is a very bright frame. Looks cool :)
    if (frame == PLAYER_FULLBRIGHTFRAME)
        frame = 32773;

    // denis - fixme - security
    if (!p->mo->sprite || (p->mo->frame & FF_FRAMEMASK) >= sprites[p->mo->sprite].numframes)
        return;

    p->last_received     = gametic;
    ::last_player_update = gametic;

    // [SL] 2012-02-21 - Save the position information to a snapshot
    int32_t        snaptime = ::last_svgametic;
    PlayerSnapshot newsnap(snaptime);
    newsnap.setAuthoritative(true);

    newsnap.setX(x);
    newsnap.setY(y);
    newsnap.setZ(z);
    newsnap.setMomX(momx);
    newsnap.setMomY(momy);
    newsnap.setMomZ(momz);
    newsnap.setAngle(angle);
    newsnap.setPitch(pitch);
    newsnap.setFrame(frame);

    // Mark the snapshot as continuous unless the player just teleported
    // and lerping should be disabled
    newsnap.setContinuous(!CL_PlayerJustTeleported(p));
    CL_ClearPlayerJustTeleported(p);

    p->snapshots.addSnapshot(newsnap);
}

static void CL_UpdateLocalPlayer(const odaproto::svc::UpdateLocalPlayer *msg)
{
    player_t &p = consoleplayer();

    // The server has processed the ticcmd that the local client sent
    // during the the tic referenced below
    p.tic = msg->tic();

    fixed_t x = msg->actor().pos().x();
    fixed_t y = msg->actor().pos().y();
    fixed_t z = msg->actor().pos().z();

    fixed_t momx = msg->actor().mom().x();
    fixed_t momy = msg->actor().mom().y();
    fixed_t momz = msg->actor().mom().z();

    uint8_t waterlevel = msg->actor().waterlevel();

    int32_t        snaptime = ::last_svgametic;
    PlayerSnapshot newsnapshot(snaptime);
    newsnapshot.setAuthoritative(true);
    newsnapshot.setX(x);
    newsnapshot.setY(y);
    newsnapshot.setZ(z);
    newsnapshot.setMomX(momx);
    newsnapshot.setMomY(momy);
    newsnapshot.setMomZ(momz);
    newsnapshot.setWaterLevel(waterlevel);

    // Mark the snapshot as continuous unless the player just teleported
    // and lerping should be disabled
    newsnapshot.setContinuous(!CL_PlayerJustTeleported(&p));
    CL_ClearPlayerJustTeleported(&p);

    consoleplayer().snapshots.addSnapshot(newsnapshot);
}

// Set level locals.
static void CL_LevelLocals(const odaproto::svc::LevelLocals *msg)
{
    uint32_t flags = msg->flags();

    if (flags & SVC_LL_TIME)
    {
        ::level.time = msg->time();
    }

    if (flags & SVC_LL_TOTALS)
    {
        ::level.total_secrets  = msg->total_secrets();
        ::level.total_items    = msg->total_items();
        ::level.total_monsters = msg->total_monsters();
    }

    if (flags & SVC_LL_SECRETS)
    {
        ::level.found_secrets = msg->found_secrets();
    }

    if (flags & SVC_LL_ITEMS)
    {
        ::level.found_items = msg->found_items();
    }

    if (flags & SVC_LL_MONSTERS)
    {
        ::level.killed_monsters = msg->killed_monsters();
    }

    if (flags & SVC_LL_MONSTER_RESPAWNS)
    {
        ::level.respawned_monsters = msg->respawned_monsters();
    }
}

//
// CL_SendPingReply
//
// Replies to a server's ping request
//
// [SL] 2011-05-11 - Changed from CL_ResendSvGametic to CL_SendPingReply
// for clarity since it sends timestamps, not gametics.
//
static void CL_PingRequest(const odaproto::svc::PingRequest *msg)
{
    MSG_WriteMarker(&net_buffer, clc_pingreply);
    MSG_WriteLong(&net_buffer, msg->ms_time());
}

//
// CL_UpdatePing
// Update ping value
//
static void CL_UpdatePing(const odaproto::svc::UpdatePing *msg)
{
    player_t &p = idplayer(msg->pid());
    if (!validplayer(p))
        return;

    p.ping = msg->ping();
}

//
// CL_SpawnMobj
//
static void CL_SpawnMobj(const odaproto::svc::SpawnMobj *msg)
{
    // Read baseline

    baseline_t base;
    {
        const odaproto::Vec3 &pos = msg->baseline().pos();
        base.pos.x                = pos.x();
        base.pos.y                = pos.y();
        base.pos.z                = pos.z();
    }
    {
        const odaproto::Vec3 &mom = msg->baseline().mom();
        base.mom.x                = mom.x();
        base.mom.y                = mom.y();
        base.mom.z                = mom.z();
    }
    base.angle     = msg->baseline().angle();
    base.targetid  = msg->baseline().targetid();
    base.tracerid  = msg->baseline().tracerid();
    base.movecount = msg->baseline().movecount();
    base.movedir   = msg->baseline().movedir();
    base.rndindex  = msg->baseline().rndindex();

    // Read other fields

    uint32_t   netid = msg->current().netid();
    mobjtype_t type  = static_cast<mobjtype_t>(msg->current().type());
    statenum_t state = static_cast<statenum_t>(msg->current().statenum());

    if (type < MT_PLAYER || type >= NUMMOBJTYPES)
        return;

    P_ClearId(netid);

    AActor *mo   = new AActor(base.pos.x, base.pos.y, base.pos.z, type);
    mo->baseline = base;

    P_SetThingId(mo, netid);

    // Assign baseline/current data to spawned mobj

    const uint32_t bflags = msg->baseline_flags();

    // If position has changed, needs a relink.
    if (bflags & (baseline_t::POSX | baseline_t::POSY | baseline_t::POSZ))
    {
        mo->UnlinkFromWorld();

        if (bflags & baseline_t::POSX)
            mo->x = msg->current().pos().x();

        if (bflags & baseline_t::POSY)
            mo->y = msg->current().pos().y();

        if (bflags & baseline_t::POSZ)
            mo->z = msg->current().pos().z();

        mo->LinkToWorld();

        if (mo->subsector)
        {
            mo->floorz      = P_FloorHeight(mo);
            mo->ceilingz    = P_CeilingHeight(mo);
            mo->dropoffz    = mo->floorz;
            mo->floorsector = mo->subsector->sector;
        }
    }

    if (bflags & baseline_t::MOMX)
        mo->momx = msg->current().mom().x();
    else
        mo->momx = base.mom.x;

    if (bflags & baseline_t::MOMY)
        mo->momy = msg->current().mom().y();
    else
        mo->momy = base.mom.y;

    if (bflags & baseline_t::MOMZ)
        mo->momz = msg->current().mom().z();
    else
        mo->momz = base.mom.z;

    if (bflags & baseline_t::ANGLE)
        mo->angle = msg->current().angle();
    else
        mo->angle = base.angle;

    AActor *target = NULL;
    if (bflags & baseline_t::TARGET)
        target = P_FindThingById(msg->current().targetid());
    else
        target = P_FindThingById(base.targetid);

    if (target)
    {
        mo->target = target->ptr();
    }
    else
    {
        mo->target = AActor::AActorPtr();
    }

    // Light up the projectile if it came from a horde boss
    // This is a hack because oflags are a hack.
    if (mo->flags & MF_MISSILE && mo->target && mo->target->oflags && (mo->target->oflags & hordeBossModMask))
    {
        mo->oflags |= MFO_FULLBRIGHT;
        mo->effects = FX_YELLOWFOUNTAIN;
    }

    AActor *tracer = NULL;
    if (bflags & baseline_t::TRACER)
        tracer = P_FindThingById(msg->current().tracerid());
    else
        tracer = P_FindThingById(base.tracerid);

    if (tracer)
    {
        mo->tracer = tracer->ptr();
    }
    else
    {
        mo->tracer = AActor::AActorPtr();
    }

    if (bflags & baseline_t::MOVECOUNT)
        mo->movecount = msg->current().movecount();
    else
        mo->movecount = base.movecount;

    if (bflags & baseline_t::MOVEDIR)
        mo->movedir = msg->current().movedir();
    else
        mo->movedir = base.movedir;

    if (bflags & baseline_t::RNDINDEX)
        mo->rndindex = msg->current().rndindex();
    else
        mo->rndindex = base.rndindex;

    // denis - puff hack
    if (mo->type == MT_PUFF)
    {
        mo->momz = FRACUNIT;
        mo->tics -= M_Random() & 3;
        if (mo->tics < 1)
            mo->tics = 1;
    }

    if (state >= S_NULL && state < NUMSTATES)
    {
        P_SetMobjState(mo, state);
    }

    if (serverside && mo->flags & MF_COUNTKILL)
    {
        level.total_monsters++;
    }

    if (connected && (mo->flags & MF_MISSILE) && mo->info->seesound)
    {
        S_Sound(mo, CHAN_VOICE, mo->info->seesound, 1, ATTN_NORM);
    }

    if (mo->type == MT_IFOG)
    {
        S_Sound(mo, CHAN_VOICE, "misc/spawn", 1, ATTN_IDLE);
    }

    if (mo->type == MT_TFOG)
    {
        if (level.time) // don't play sound on first tic of the level
        {
            S_Sound(mo, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
        }
    }

    if (type == MT_FOUNTAIN)
    {
        if (msg->args_size() >= 1)
            mo->effects = msg->args().Get(0) << FX_FOUNTAINSHIFT;
    }

    if (type == MT_ZDOOMBRIDGE)
    {
        if (msg->args_size() >= 1)
            mo->radius = msg->args().Get(0) << FRACBITS;
        if (msg->args_size() >= 2)
            mo->height = msg->args().Get(1) << FRACBITS;
    }

    if (msg->spawn_flags() & SVC_SM_FLAGS)
    {
        mo->flags = msg->current().flags();
    }

    if (msg->spawn_flags() & SVC_SM_OFLAGS)
    {
        mo->oflags = msg->current().oflags();

        // [AM] HACK! Assume that any monster with a flag is a boss.
        if (mo->oflags)
        {
            mo->effects = FX_YELLOWFOUNTAIN;
        }
    }

    if (msg->spawn_flags() & SVC_SM_CORPSE)
    {
        int32_t frame = msg->current().frame();
        int32_t tics  = msg->current().tics();

        if (tics == 0xFF)
            tics = -1;

        // already spawned as gibs?
        if (!mo || mo->state - states == S_GIBS)
            return;

        if ((frame & FF_FRAMEMASK) >= sprites[mo->sprite].numframes)
            return;

        mo->frame = frame;
        mo->tics  = tics;

        // from P_KillMobj
        mo->flags &= ~(MF_SHOOTABLE | MF_FLOAT | MF_SKULLFLY);
        mo->flags |= MF_CORPSE | MF_DROPOFF;
        mo->height >>= 2;
        mo->flags &= ~MF_SOLID;
        if (mo->oflags & hordeBossModMask)
        {
            mo->effects = 0; // Remove sparkles from boss corpses
        }

        if (mo->player)
            mo->player->playerstate = PST_DEAD;
    }
}

//
// CL_DisconnectClient
//
static void CL_DisconnectClient(const odaproto::svc::DisconnectClient *msg)
{
    player_t &player = idplayer(msg->pid());
    if (players.empty() || !validplayer(player))
        return;

    if (player.mo)
    {
        P_DisconnectEffect(player.mo);

        // [AM] Destroying the player mobj is not our responsibility.  However, we do want
        //      to make sure that the mobj->player doesn't point to an invalid player.
        player.mo->player = NULL;
    }

    // Remove the player from the players list.
    for (Players::iterator it = players.begin(); it != players.end(); ++it)
    {
        if (it->id == player.id)
        {
            if (::cl_disconnectalert && &player != &consoleplayer())
                S_Sound(CHAN_INTERFACE, "misc/plpart", 1, ATTN_NONE);
            if (!it->spectator)
                P_PlayerLeavesGame(&(*it));
            players.erase(it);
            break;
        }
    }

    // if this was our displayplayer, update camera
    CL_CheckDisplayPlayer();
}

//
// LoadMap
//
// Read wad & deh filenames and map name from the server and loads
// the appropriate wads & map.
//
static void CL_LoadMap(const odaproto::svc::LoadMap *msg)
{
    ClientReplay::getInstance().reset();

    size_t     wadcount = msg->wadnames_size();
    OWantFiles newwadfiles;
    newwadfiles.reserve(wadcount);
    for (size_t i = 0; i < wadcount; i++)
    {
        std::string name    = msg->wadnames().Get(i).name();
        std::string hashStr = msg->wadnames().Get(i).hash();

        OMD5Hash hash;
        OMD5Hash::makeFromHexStr(hash, hashStr);

        OWantFile file;
        if (!OWantFile::makeWithHash(file, name, hash))
        {
            Printf(PRINT_WARNING, "Could not construct wanted file \"%s\" that server requested.\n", name.c_str());
            CL_QuitNetGame(NQ_DISCONNECT);
            return;
        }
        newwadfiles.push_back(file);
    }

    std::string mapname           = msg->mapname();
    int32_t     server_level_time = msg->time();

    // Load the specified WAD files and change the level.
    // if any WADs are missing, reconnect to begin downloading.
    G_LoadWad(newwadfiles);

    if (!missingfiles.empty())
    {
        OWantFile missing_file = missingfiles.front();
        CL_QuitAndTryDownload(missing_file);
        return;
    }

    // [SL] 2012-12-02 - Force the music to stop when the new map uses
    // the same music lump name that is currently playing. Otherwise,
    // the music from the old wad continues to play...
    S_StopMusic();

    G_InitNew(mapname.c_str());

    // [AM] Sync the server's level time with the client.
    ::level.time = server_level_time;

    ::movingsectors.clear();
    ::teleported_players.clear();

    CL_ClearSectorSnapshots();
    for (Players::iterator it = players.begin(); it != players.end(); ++it)
        it->snapshots.clearSnapshots();

    // reset the world_index (force it to sync)
    CL_ResyncWorldIndex();
    ::last_svgametic = 0;

    gameaction = ga_nothing;
}

static void CL_ConsolePlayer(const odaproto::svc::ConsolePlayer *msg)
{
    ::displayplayer_id = ::consoleplayer_id = msg->pid();
    ::digest                                = msg->digest();
}

//
// CL_ExplodeMissile
//
static void CL_ExplodeMissile(const odaproto::svc::ExplodeMissile *msg)
{
    AActor *mo = P_FindThingById(msg->netid());

    if (!mo)
        return;

    P_ExplodeMissile(mo);
}

//
// CL_RemoveMobj
//
static void CL_RemoveMobj(const odaproto::svc::RemoveMobj *msg)
{
    uint32_t netid = msg->netid();

    AActor *mo = P_FindThingById(netid);
    if (mo && mo->player && mo->player->id == ::displayplayer_id)
        ::displayplayer_id = ::consoleplayer_id;

    if (mo && mo->flags & MF_COUNTITEM)
        level.found_items++;

    P_ClearId(netid);
}

//
// CL_SetupUserInfo
//
static void CL_UserInfo(const odaproto::svc::UserInfo *msg)
{
    player_t *p = &CL_FindPlayer(msg->pid());

    p->userinfo.netname = msg->netname();

    p->userinfo.team = static_cast<team_t>(msg->team());
    if (p->userinfo.team < 0 || p->userinfo.team >= NUMTEAMS)
        p->userinfo.team = TEAM_BLUE;

    p->userinfo.gender = static_cast<gender_t>(msg->gender());
    if (p->userinfo.gender < 0 || p->userinfo.gender >= NUMGENDER)
        p->userinfo.gender = GENDER_NEUTER;

    p->userinfo.color[0] = 255;
    p->userinfo.color[1] = msg->color().r();
    p->userinfo.color[2] = msg->color().g();
    p->userinfo.color[3] = msg->color().b();

    p->GameTime = msg->join_time();

    // [SL] 2012-04-30 - Were we looking through a teammate's POV who changed
    // to the other team?
    // [SL] 2012-05-24 - Were we spectating a teammate before we changed teams?
    CL_CheckDisplayPlayer();
}

static void CL_UpdateMobj(const odaproto::svc::UpdateMobj *msg)
{
    AActor *mo = P_FindThingById(msg->actor().netid());
    if (!mo)
        return;

    uint32_t flags = msg->flags();

    baseline_t update = mo->baseline;
    if (flags & baseline_t::POSX)
    {
        update.pos.x = msg->actor().pos().x();
    }
    if (flags & baseline_t::POSY)
    {
        update.pos.y = msg->actor().pos().y();
    }
    if (flags & baseline_t::POSZ)
    {
        update.pos.z = msg->actor().pos().z();
    }
    if (flags & baseline_t::ANGLE)
    {
        update.angle = msg->actor().angle();
    }
    if (flags & baseline_t::MOVEDIR)
    {
        update.movedir = msg->actor().movedir();
    }
    if (flags & baseline_t::MOVECOUNT)
    {
        update.movecount = msg->actor().movecount();
    }
    if (flags & baseline_t::RNDINDEX)
    {
        update.rndindex = msg->actor().rndindex();
    }
    if (flags & baseline_t::TARGET)
    {
        update.targetid = msg->actor().targetid();
    }
    if (flags & baseline_t::TRACER)
    {
        update.tracerid = msg->actor().tracerid();
    }

    if (flags & baseline_t::MOMX)
    {
        update.mom.x = msg->actor().mom().x();
    }
    if (flags & baseline_t::MOMY)
    {
        update.mom.y = msg->actor().mom().y();
    }
    if (flags & baseline_t::MOMZ)
    {
        update.mom.z = msg->actor().mom().z();
    }

    if (mo->player)
    {
        // [SL] 2013-07-21 - Save the position information to a snapshot
        int32_t        snaptime = last_svgametic;
        PlayerSnapshot newsnap(snaptime);
        newsnap.setAuthoritative(true);

        newsnap.setX(update.pos.x);
        newsnap.setY(update.pos.y);
        newsnap.setZ(update.pos.z);
        newsnap.setAngle(update.angle);
        newsnap.setMomX(update.mom.x);
        newsnap.setMomY(update.mom.y);
        newsnap.setMomZ(update.mom.z);

        mo->player->snapshots.addSnapshot(newsnap);
    }
    else
    {
        CL_MoveThing(mo, update.pos.x, update.pos.y, update.pos.z);
        mo->angle = update.angle;
        mo->momx  = update.mom.x;
        mo->momy  = update.mom.y;
        mo->momz  = update.mom.z;
    }

    mo->rndindex  = update.rndindex;
    mo->movedir   = update.movedir;
    mo->movecount = update.movecount;

    AActor *target = P_FindThingById(update.targetid);
    if (target)
        mo->target = target->ptr();
    else
        mo->target = AActor::AActorPtr();

    AActor *tracer = P_FindThingById(update.tracerid);
    if (tracer)
        mo->tracer = tracer->ptr();
    else
        mo->tracer = AActor::AActorPtr();
}

//
// CL_SpawnPlayer
//
static void CL_SpawnPlayer(const odaproto::svc::SpawnPlayer *msg)
{
    size_t    playernum = msg->pid();
    size_t    netid     = msg->actor().netid();
    player_t *p         = &CL_FindPlayer(playernum);

    angle_t angle = msg->actor().angle();
    fixed_t x     = msg->actor().pos().x();
    fixed_t y     = msg->actor().pos().y();
    fixed_t z     = msg->actor().pos().z();

    P_ClearId(netid);

    // first disassociate the corpse
    if (p->mo)
    {
        p->mo->player = NULL;
        p->mo->health = 0;
    }

    G_PlayerReborn(*p);

    AActor *mobj = new AActor(x, y, z, MT_PLAYER);

    mobj->momx = mobj->momy = mobj->momz = 0;

    // set color translations for player sprites
    mobj->angle  = angle;
    mobj->pitch  = 0;
    mobj->player = p;
    mobj->health = p->health;
    P_SetThingId(mobj, netid);

    p->mo = p->camera = mobj->ptr();
    p->fov            = 90.0f;
    p->playerstate    = PST_LIVE;
    p->refire         = 0;
    p->damagecount    = 0;
    p->bonuscount     = 0;
    p->extralight     = 0;
    p->fixedcolormap  = 0;

    p->xviewshift = 0;
    p->viewheight = VIEWHEIGHT;

    p->attacker = AActor::AActorPtr();
    p->viewz    = z + VIEWHEIGHT;

    // spawn a teleport fog
    // tfog = new AActor (x, y, z, MT_TFOG);

    // setup gun psprite
    P_SetupPsprites(p);

    // give all cards in death match mode
    if (!G_IsCoopGame())
        for (size_t i = 0; i < NUMCARDS; i++)
            p->cards[i] = true;

    if (p->id == consoleplayer_id)
    {
        // flash taskbar icon
        IWindow *window = I_GetWindow();
        window->flashWindow();

        // [SL] 2012-04-23 - Clear predicted sectors
        movingsectors.clear();
    }

    if (p->id == displayplayer().id)
    {
        // [SL] 2012-03-08 - Resync with the server's incoming tic since we don't care
        // about players/sectors jumping to new positions when the displayplayer spawns
        CL_ResyncWorldIndex();
    }

    if (level.behavior && !p->spectator && p->playerstate == PST_LIVE)
    {
        if (p->deathcount)
            ::level.behavior->StartTypedScripts(SCRIPT_Respawn, p->mo);
        else
            ::level.behavior->StartTypedScripts(SCRIPT_Enter, p->mo);
    }

    int32_t        snaptime = last_svgametic;
    PlayerSnapshot newsnap(snaptime, p);
    newsnap.setAuthoritative(true);
    newsnap.setContinuous(false);
    p->snapshots.clearSnapshots();
    p->snapshots.addSnapshot(newsnap);
}

//
// CL_DamagePlayer
//
static void CL_DamagePlayer(const odaproto::svc::DamagePlayer *msg)
{
    uint32_t netid        = msg->netid();
    uint32_t attackerid   = msg->inflictorid();
    int32_t  healthDamage = msg->health_damage();
    int32_t  armorDamage  = msg->armor_damage();
    int32_t  health       = msg->player().health();
    int32_t  armorpoints  = msg->player().armorpoints();

    AActor *actor    = P_FindThingById(netid);
    AActor *attacker = P_FindThingById(attackerid);

    if (!actor || !actor->player)
        return;

    player_t *p    = actor->player;
    p->health      = MIN(p->health, health);
    p->armorpoints = MIN(p->armorpoints, armorpoints);
    p->mo->health  = p->health;

    if (attacker != NULL)
        p->attacker = attacker->ptr();

    if (p->health < 0)
    {
        if (p->cheats & CF_BUDDHA)
        {
            p->health     = 1;
            p->mo->health = 1;
        }
        else
            p->health = 0;
    }

    if (p->armorpoints < 0)
        p->armorpoints = 0;

    if (p->armorpoints == 0)
        p->armortype = 0;

    if (healthDamage > 0)
    {
        p->damagecount += healthDamage;

        if (p->damagecount > 100)
            p->damagecount = 100;

        if (p->mo->info->painstate)
            P_SetMobjState(p->mo, p->mo->info->painstate);
    }
}

extern int32_t MeansOfDeath;

//
// CL_KillMobj
//
static void CL_KillMobj(const odaproto::svc::KillMobj *msg)
{
    uint32_t srcid  = msg->source_netid();
    uint32_t tgtid  = msg->target().netid();
    uint32_t infid  = msg->inflictor_netid();
    int32_t  health = msg->health();
    //::MeansOfDeath = msg->mod();
    bool    joinkill = msg->joinkill();
    int32_t lives    = msg->lives();

    AActor *source    = P_FindThingById(srcid);
    AActor *target    = P_FindThingById(tgtid);
    AActor *inflictor = P_FindThingById(infid);

    if (!target)
        return;

    // This used to be bundled with a svc_movemobj and svc_mobjspeedangle,
    // so emulate them here.
    target->rndindex = msg->target().rndindex();

    if (target->player)
    {
        // [SL] 2013-07-21 - Save the position information to a snapshot
        int32_t        snaptime = last_svgametic;
        PlayerSnapshot newsnap(snaptime);
        newsnap.setAuthoritative(true);

        newsnap.setX(msg->target().pos().x());
        newsnap.setY(msg->target().pos().y());
        newsnap.setZ(msg->target().pos().z());
        newsnap.setAngle(msg->target().angle());
        newsnap.setMomX(msg->target().mom().x());
        newsnap.setMomY(msg->target().mom().y());
        newsnap.setMomZ(msg->target().mom().z());

        target->player->snapshots.addSnapshot(newsnap);
    }
    else
    {
        target->x     = msg->target().pos().x();
        target->y     = msg->target().pos().y();
        target->z     = msg->target().pos().z();
        target->angle = msg->target().angle();
        target->momx  = msg->target().mom().x();
        target->momy  = msg->target().mom().y();
        target->momz  = msg->target().mom().z();
    }

    target->health = health;

    if (!serverside && target->flags & MF_COUNTKILL)
        level.killed_monsters++;

    if (target->player == &consoleplayer())
    {
        ClientReplay::getInstance().reset();
        for (size_t i = 0; i < MAXSAVETICS; i++)
            localcmds[i].clear();
    }

    if (target->player && lives >= 0)
        target->player->lives = lives;

    P_KillMobj(source, target, inflictor, joinkill);
}

///////////////////////////////////////////////////////////
///// CL_Fire* called when someone uses a weapon  /////////
///////////////////////////////////////////////////////////

// [tm512] attempt at squashing weapon desyncs.
// The server will send us what weapon we fired, and if that
// doesn't match the weapon we have up at the moment, fix it
// and request that we get a full update of playerinfo - apr 14 2012
static void CL_FireWeapon(const odaproto::svc::FireWeapon *msg)
{
    player_t *p = &consoleplayer();

    weapontype_t firedweap = static_cast<weapontype_t>(msg->readyweapon());
    if (firedweap < 0 || firedweap > wp_nochange)
    {
        Printf("CL_FireWeapon: unknown weapon %d\n", firedweap);
        return;
    }
    int32_t servertic = msg->servertic();

    if (firedweap != p->readyweapon)
    {
        DPrintf("CL_FireWeapon: weapon misprediction\n");
        A_ForceWeaponFire(p->mo, firedweap, servertic);

        // Request the player's ammo status from the server
        MSG_WriteMarker(&net_buffer, clc_getplayerinfo);
    }
}

//
// CL_UpdateSector
// Updates floorheight and ceilingheight of a sector.
//
static void CL_UpdateSector(const odaproto::svc::UpdateSector *msg)
{
    int32_t     sectornum     = msg->sectornum();
    fixed_t     floorheight   = msg->sector().floor_height();
    fixed_t     ceilingheight = msg->sector().ceiling_height();
    texhandle_t floorpic      = msg->sector().floorpic();
    texhandle_t ceilingpic    = msg->sector().ceilingpic();
    int32_t     special       = msg->sector().special();

    if (!::sectors || sectornum < 0 || sectornum >= ::numsectors)
        return;

    sector_t *sector = &::sectors[sectornum];
    P_SetCeilingHeight(sector, ceilingheight);
    P_SetFloorHeight(sector, floorheight);

    sector->floorpic   = floorpic;
    sector->ceilingpic = ceilingpic;
    sector->special    = special;
    sector->moveable   = true;

    P_ChangeSector(sector, false);

    SectorSnapshot snap(last_svgametic, sector);
    sector_snaps[sectornum].addSnapshot(snap);
}

//
// CL_Print
//
static void CL_Print(const odaproto::svc::Print *msg)
{
    uint8_t     level = msg->level();
    const char *str   = msg->message().c_str();

    // Disallow getting NORCON messages
    if (level == PRINT_NORCON)
        return;

    // TODO : Clientchat moved, remove that but PRINT_SERVERCHAT
    if (level == PRINT_CHAT)
        Printf(level, "%s*%s", TEXTCOLOR_ESCAPE, str);
    else if (level == PRINT_TEAMCHAT)
        Printf(level, "%s!%s", TEXTCOLOR_ESCAPE, str);
    else if (level == PRINT_SERVERCHAT)
        Printf(level, "%s%s", TEXTCOLOR_YELLOW, str);
    else
        Printf(level, "%s", str);

    if (show_messages)
    {
        if (level == PRINT_CHAT || level == PRINT_SERVERCHAT)
            S_Sound(CHAN_INTERFACE, gameinfo.chatSound, 1, ATTN_NONE);
        else if (level == PRINT_TEAMCHAT)
            S_Sound(CHAN_INTERFACE, "misc/teamchat", 1, ATTN_NONE);
    }
}

/**
 * @brief Updates less-vital members of a player struct.
 */
static void CL_PlayerMembers(const odaproto::svc::PlayerMembers *msg)
{
    player_t &p     = CL_FindPlayer(msg->pid());
    uint8_t   flags = msg->flags();

    if (flags & SVC_PM_SPECTATOR)
    {
        CL_SpectatePlayer(p, msg->spectator());
    }

    if (flags & SVC_PM_READY)
    {
        p.ready = msg->ready();
    }

    if (flags & SVC_PM_LIVES)
    {
        p.lives = msg->lives();
    }

    if (flags & SVC_PM_DAMAGE)
    {
        p.monsterdmgcount = msg->monsterdmgcount();
    }

    if (flags & SVC_PM_SCORE)
    {
        p.roundwins   = msg->roundwins();
        p.points      = msg->points();
        p.fragcount   = msg->fragcount();
        p.deathcount  = msg->deathcount();
        p.killcount   = msg->killcount();
        p.secretcount = msg->secretcount();
        p.totalpoints = msg->totalpoints();
        p.totaldeaths = msg->totaldeaths();
    }

    if (flags & SVC_PM_CHEATS)
    {
        if (!p.spectator)
            p.cheats = msg->cheats();
    }
}

//
// [deathz0r] Receive team frags/captures
//
static void CL_TeamMembers(const odaproto::svc::TeamMembers *msg)
{
    team_t  team      = static_cast<team_t>(msg->team());
    int32_t points    = msg->points();
    int32_t roundWins = msg->roundwins();

    // Ensure our team is valid.
    TeamInfo *info = GetTeamInfo(team);
    if (info->Team >= NUMTEAMS)
        return;

    info->Points    = points;
    info->RoundWins = roundWins;
}

static void CL_ActivateLine(const odaproto::svc::ActivateLine *msg)
{
    int32_t            linenum        = msg->linenum();
    AActor            *mo             = P_FindThingById(msg->activator_netid());
    uint8_t            side           = msg->side();
    LineActivationType activationType = static_cast<LineActivationType>(msg->activation_type());
    const bool         bossaction     = msg->bossaction();

    if (!::lines || linenum >= ::numlines || linenum < 0)
        return;

    ActivateLine(mo, &::lines[linenum], side, activationType, bossaction);
}

//
// CL_UpdateMovingSector
// Updates floorheight and ceilingheight of a sector.
//
static void CL_MovingSector(const odaproto::svc::MovingSector *msg)
{
    int32_t sectornum = msg->sector();

    fixed_t ceilingheight = msg->ceiling_height();
    fixed_t floorheight   = msg->floor_height();

    uint32_t    movers        = msg->movers();
    movertype_t ceiling_mover = static_cast<movertype_t>(movers & BIT_MASK(0, 3));
    movertype_t floor_mover   = static_cast<movertype_t>((movers & BIT_MASK(4, 7)) >> 4);

    if (ceiling_mover == SEC_ELEVATOR)
    {
        floor_mover = SEC_INVALID;
    }
    if (ceiling_mover == SEC_PILLAR)
    {
        floor_mover = SEC_INVALID;
    }

    SectorSnapshot snap(::last_svgametic);

    snap.setCeilingHeight(ceilingheight);
    snap.setFloorHeight(floorheight);

    if (floor_mover == SEC_FLOOR)
    {
        const odaproto::svc::MovingSector_Snapshot &floor = msg->floor_mover();

        // Floors/Stairbuilders
        snap.setFloorMoverType(SEC_FLOOR);
        snap.setFloorType(static_cast<DFloor::EFloor>(floor.floor_type()));
        snap.setFloorStatus(floor.floor_status());
        snap.setFloorCrush(floor.floor_crush());
        snap.setFloorDirection(floor.floor_dir());
        snap.setFloorSpecial(floor.floor_speed());
        snap.setFloorTexture(floor.floor_tex());
        snap.setFloorDestination(floor.floor_dest());
        snap.setFloorSpeed(floor.floor_speed());
        snap.setResetCounter(floor.reset_counter());
        snap.setOrgHeight(floor.orig_height());
        snap.setDelay(floor.delay());
        snap.setPauseTime(floor.pause_time());
        snap.setStepTime(floor.step_time());
        snap.setPerStepTime(floor.per_step_time());
        snap.setFloorOffset(floor.floor_offset());
        snap.setFloorChange(floor.floor_change());

        int32_t LineIndex = floor.floor_line();

        if (!lines || LineIndex >= numlines || LineIndex < 0)
            snap.setFloorLine(NULL);
        else
            snap.setFloorLine(&lines[LineIndex]);
    }

    if (floor_mover == SEC_PLAT)
    {
        const odaproto::svc::MovingSector_Snapshot &floor = msg->floor_mover();

        // Platforms/Lifts
        snap.setFloorMoverType(SEC_PLAT);
        snap.setFloorSpeed(floor.floor_speed());
        snap.setFloorLow(floor.floor_low());
        snap.setFloorHigh(floor.floor_high());
        snap.setFloorWait(floor.floor_wait());
        snap.setFloorCounter(floor.floor_counter());
        snap.setFloorStatus(floor.floor_status());
        snap.setOldFloorStatus(floor.floor_old_status());
        snap.setFloorCrush(floor.floor_crush());
        snap.setFloorTag(floor.floor_tag());
        snap.setFloorType(floor.floor_type());
        snap.setFloorOffset(floor.floor_offset());
        snap.setFloorLip(floor.floor_lip());
    }

    if (ceiling_mover == SEC_CEILING)
    {
        const odaproto::svc::MovingSector_Snapshot &ceil = msg->ceiling_mover();

        // Ceilings / Crushers
        snap.setCeilingMoverType(SEC_CEILING);
        snap.setCeilingType(ceil.ceil_type());
        snap.setCeilingLow(ceil.ceil_low());
        snap.setCeilingHigh(ceil.ceil_high());
        snap.setCeilingSpeed(ceil.ceil_speed());
        snap.setCrusherSpeed1(ceil.crusher_speed_1());
        snap.setCrusherSpeed2(ceil.crusher_speed_2());
        snap.setCeilingCrush(ceil.ceil_crush());
        snap.setSilent(ceil.silent());
        snap.setCeilingDirection(ceil.ceil_dir());
        snap.setCeilingTexture(ceil.ceil_tex());
        snap.setCeilingSpecial(ceil.ceil_new_special());
        snap.setCeilingTag(ceil.ceil_tag());
        snap.setCeilingOldDirection(ceil.ceil_old_dir());
    }

    if (ceiling_mover == SEC_DOOR)
    {
        const odaproto::svc::MovingSector_Snapshot &ceil = msg->ceiling_mover();

        // Doors
        snap.setCeilingMoverType(SEC_DOOR);
        snap.setCeilingType(static_cast<DDoor::EVlDoor>(ceil.ceil_type()));
        snap.setCeilingHigh(ceil.ceil_height());
        snap.setCeilingSpeed(ceil.ceil_speed());
        snap.setCeilingWait(ceil.ceil_wait());
        snap.setCeilingCounter(ceil.ceil_counter());
        snap.setCeilingStatus(ceil.ceil_status());

        int32_t LineIndex = ceil.ceil_line();

        // If the moving sector's line is -1, it is likely a type 666 door
        if (!lines || LineIndex >= numlines || LineIndex < 0)
            snap.setCeilingLine(NULL);
        else
            snap.setCeilingLine(&lines[LineIndex]);
    }

    if (ceiling_mover == SEC_ELEVATOR)
    {
        const odaproto::svc::MovingSector_Snapshot &ceil = msg->ceiling_mover();

        // Elevators
        snap.setCeilingMoverType(SEC_ELEVATOR);
        snap.setFloorMoverType(SEC_ELEVATOR);
        snap.setCeilingType(static_cast<DElevator::EElevator>(ceil.ceil_type()));
        snap.setFloorType(snap.getCeilingType());
        snap.setCeilingStatus(ceil.ceil_status());
        snap.setFloorStatus(snap.getCeilingStatus());
        snap.setCeilingDirection(ceil.ceil_dir());
        snap.setFloorDirection(snap.getCeilingDirection());
        snap.setFloorDestination(ceil.floor_dest());
        snap.setCeilingDestination(ceil.ceil_dest());
        snap.setCeilingSpeed(ceil.ceil_speed());
        snap.setFloorSpeed(snap.getCeilingSpeed());
    }

    if (ceiling_mover == SEC_PILLAR)
    {
        const odaproto::svc::MovingSector_Snapshot &ceil = msg->ceiling_mover();

        // Pillars
        snap.setCeilingMoverType(SEC_PILLAR);
        snap.setFloorMoverType(SEC_PILLAR);
        snap.setCeilingType(static_cast<DPillar::EPillar>(ceil.ceil_type()));
        snap.setFloorType(snap.getCeilingType());
        snap.setCeilingStatus(ceil.ceil_status());
        snap.setFloorStatus(snap.getCeilingStatus());
        snap.setFloorSpeed(ceil.floor_speed());
        snap.setCeilingSpeed(ceil.ceil_speed());
        snap.setFloorDestination(ceil.floor_dest());
        snap.setCeilingDestination(ceil.ceil_dest());
        snap.setCeilingCrush(ceil.ceil_crush());
        snap.setFloorCrush(snap.getCeilingCrush());
    }

    if (!::sectors || sectornum < 0 || sectornum >= ::numsectors)
        return;

    snap.setSector(&::sectors[sectornum]);

    sector_snaps[sectornum].addSnapshot(snap);
}

//
// CL_Sound
//
static void CL_PlaySound(const odaproto::svc::PlaySound *msg)
{
    int32_t channel     = msg->channel();
    int32_t sfx_id      = msg->sfxid();
    float   volume      = msg->volume();
    int32_t attenuation = msg->attenuation();

    switch (msg->source_case())
    {
    case odaproto::svc::PlaySound::SOURCE_NOT_SET:
        S_SoundID(channel, sfx_id, volume, attenuation);
        return;
    case odaproto::svc::PlaySound::kNetid: {
        // play at thing location
        AActor *mo = P_FindThingById(msg->netid());
        if (!mo)
            return;

        S_SoundID(mo, channel, sfx_id, volume, attenuation);
        return;
    }
    case odaproto::svc::PlaySound::kPos: {
        // play at approximate thing location
        S_SoundID(msg->pos().x(), msg->pos().y(), channel, sfx_id, volume, attenuation);
        return;
    }
    }
}

static void CL_Reconnect(const odaproto::svc::Reconnect *msg)
{
    CL_Reconnect();
}

static void CL_ExitLevel(const odaproto::svc::ExitLevel *msg)
{
    gameaction = ga_completed;

    ClientReplay::getInstance().reset();
}

static void CL_TouchSpecial(const odaproto::svc::TouchSpecial *msg)
{
    uint32_t id = msg->netid();
    AActor  *mo = P_FindThingById(id);

    if (!consoleplayer().mo)
        return;

    if (!mo)
    {
        // Record this item into the replay engine for future replaying
        ClientReplay::getInstance().recordReplayItem(::last_svgametic, id);
        return;
    }

    P_GiveSpecial(&consoleplayer(), mo);
}

// ---------------------------------------------------------------------------------------------------------
//	CL_ForceSetTeam
//	Allows server to force set a players team setting
// ---------------------------------------------------------------------------------------------------------

static void CL_ForceTeam(const odaproto::svc::ForceTeam *msg)
{
    team_t t = static_cast<team_t>(msg->team());

    if (t < NUMTEAMS || t == TEAM_NONE)
    {
        consoleplayer().userinfo.team = t;
    }

    // Setting the cl_team will send a playerinfo packet back to the server.
    // Unfortunately, this is unavoidable until we rework the team system.
    cl_team.Set(GetTeamInfo(consoleplayer().userinfo.team)->ColorStringUpper.c_str());
}

//
// CL_Switch
// denis - switch state and timing
// Note: this will also be called for doors
static void CL_Switch(const odaproto::svc::Switch *msg)
{
    int32_t  l            = msg->linenum();
    uint8_t  switchactive = msg->switch_active();
    uint32_t special      = msg->special();
    uint32_t state        = msg->state(); // DActiveButton::EWhere
    int16_t  texture      = msg->button_texture();
    uint32_t time         = msg->timer();

    if (!::lines || l < 0 || l >= ::numlines || state >= 3)
        return;

    // denis - fixme - security
    if (!P_SetButtonInfo(&lines[l], state, time) && switchactive)
    {
        // only playsound if we've received the full update from
        // the server (not setting up the map from the server)
        bool repeat;

        if (map_format.getZDoom())
            repeat = lines[l].flags & ML_REPEATSPECIAL;
        else
            repeat = P_IsSpecialBoomRepeatable(lines[l].special);

        P_ChangeSwitchTexture(&lines[l], repeat, recv_full_update);
    }

    // Only accept texture change from server while receiving the full update
    // - this is to fix warmup switch desyncs
    if (!recv_full_update && texture)
    {
        P_SetButtonTexture(&lines[l], texture);
    }
    lines[l].special = special;
}

/**
 * Handle the svc_say server message, which contains a message from another
 * client with a player id attached to it.
 */
static void CL_Say(const odaproto::svc::Say *msg)
{
    uint8_t     message_visibility = msg->visibility();
    uint8_t     player_id          = msg->pid();
    const char *message            = msg->message().c_str();

    bool filtermessage = false;

    player_t &player = idplayer(player_id);

    if (!validplayer(player))
        return;

    bool spectator = player.spectator || player.playerstate == PST_DOWNLOAD;

    if (consoleplayer().id != player.id)
    {
        if (spectator && mute_spectators)
            filtermessage = true;

        if (mute_enemies && !spectator &&
            (G_IsFFAGame() || (G_IsTeamGame() && player.userinfo.team != consoleplayer().userinfo.team)))
            filtermessage = true;
    }

    const char  *name          = player.userinfo.netname.c_str();
    printlevel_t publicmsg     = filtermessage ? PRINT_FILTERCHAT : PRINT_CHAT;
    printlevel_t publicteammsg = filtermessage ? PRINT_FILTERCHAT : PRINT_TEAMCHAT;

    if (message_visibility == 0)
    {
        if (strnicmp(message, "/me ", 4) == 0)
            Printf(publicmsg, "* %s %s\n", name, &message[4]);
        else
            Printf(publicmsg, "%s: %s\n", name, message);

        if (show_messages && !filtermessage)
        {
            if (cl_chatsounds == 1)
                S_Sound(CHAN_INTERFACE, gameinfo.chatSound, 1, ATTN_NONE);
        }
    }
    else if (message_visibility == 1)
    {
        if (strnicmp(message, "/me ", 4) == 0)
            Printf(publicteammsg, "* %s %s\n", name, &message[4]);
        else
            Printf(publicteammsg, "%s: %s\n", name, message);

        if (show_messages && cl_chatsounds && !filtermessage)
            S_Sound(CHAN_INTERFACE, "misc/teamchat", 1, ATTN_NONE);
    }
}

//
// CL_SecretEvent
// Client interpretation of a secret found by another player
//
static void CL_SecretEvent(const odaproto::svc::SecretEvent *msg)
{
    player_t &player    = idplayer(msg->pid());
    size_t    sectornum = msg->sectornum();
    int16_t   special   = msg->sector().special();

    if (!::sectors || sectornum >= numsectors)
        return;

    sector_t *sector = &::sectors[sectornum];
    sector->flags &= ~SECF_SECRET;
    sector->secretsector = false;

    if (!map_format.getZDoom())
        if (sector->special < 32)
            sector->special = 0;

    // Don't show other secrets if requested
    if (!::hud_revealsecrets || ::hud_revealsecrets > 2)
        return;

    std::string buf;
    StrFormat(buf, "%s%s %sfound a secret!\n", TEXTCOLOR_YELLOW, player.userinfo.netname.c_str(), TEXTCOLOR_NORMAL);
    Printf("%s", buf.c_str());

    if (::hud_revealsecrets == 1)
        S_Sound(CHAN_INTERFACE, "misc/secret", 1, ATTN_NONE);
}

static void CL_ServerSettings(const odaproto::svc::ServerSettings *msg)
{
    cvar_t *var = NULL, *prev = NULL;

    std::string CvarKey   = msg->key();
    std::string CvarValue = msg->value();

    var = cvar_t::FindCVar(CvarKey.c_str(), &prev);

    // GhostlyDeath <June 19, 2008> -- Read CVAR or dump it
    if (var)
    {
        if (var->flags() & CVAR_SERVERINFO)
            var->Set(CvarValue.c_str());
    }
    else
    {
        // [Russell] - create a new "temporary" cvar, CVAR_AUTO marks it
        // for cleanup on program termination
        // [AM] We have no way of telling of cvars are CVAR_NOENABLEDISABLE,
        //      so let's set it on all cvars.
        var = new cvar_t(CvarKey.c_str(), NULL, "", CVARTYPE_NONE,
                         CVAR_SERVERINFO | CVAR_AUTO | CVAR_UNSETTABLE | CVAR_NOENABLEDISABLE);
        var->Set(CvarValue.c_str());
    }

    // Nes - update the skies in case sv_freelook is changed.
    // Do we need this now that (allowing) freelook is unconditional? - Dasho
    R_InitSkyMap();
}

//
// CL_ConnectClient
//
static void CL_ConnectClient(const odaproto::svc::ConnectClient *msg)
{
    player_t &player = idplayer(msg->pid());

    CL_CheckDisplayPlayer();

    if (!::cl_connectalert)
        return;

    // GhostlyDeath <August 1, 2008> -- Play connect sound
    if (&player == &consoleplayer())
        return;

    S_Sound(CHAN_INTERFACE, "misc/pljoin", 1, ATTN_NONE);
}

// Print a message in the middle of the screen
static void CL_MidPrint(const odaproto::svc::MidPrint *msg)
{
    // C_MidPrint(msg->message().c_str(), NULL, msg->time());
}

//
// CL_SaveSvGametic
//
// Receives the server's gametic at the time the packet was sent.  It will be
// sent back to the server with the next cmd.
//
// [SL] 2011-05-11
static void CL_ServerGametic(const odaproto::svc::ServerGametic *msg)
{
    uint8_t t = msg->tic();

    int32_t newtic = (::last_svgametic & 0xFFFFFF00) + t;

    if (::last_svgametic > newtic + 127)
        newtic += 256;

    ::last_svgametic = newtic;

#ifdef _WORLD_INDEX_DEBUG_
    Printf(PRINT_HIGH, "Gametic %i, received world index %i\n", gametic, last_svgametic);
#endif // _WORLD_INDEX_DEBUG_
}

//
// CL_UpdateIntTimeLeft
// Changes the value of level.inttimeleft
//
static void CL_IntTimeLeft(const odaproto::svc::IntTimeLeft *msg)
{
    ::level.inttimeleft = msg->timeleft(); // convert from seconds to tics
}

//
// CL_FullUpdateDone
//
// Takes care of any business that needs to be done once the client has a full
// view of the game world.
//
static void CL_FullUpdateDone(const odaproto::svc::FullUpdateDone *msg)
{
    ::recv_full_update = true;
}

//
// CL_RailTrail
//
static void CL_RailTrail(const odaproto::svc::RailTrail *msg)
{
    v3double_t start, end;

    start.x = FIXED2DOUBLE(msg->start().x());
    start.y = FIXED2DOUBLE(msg->start().y());
    start.z = FIXED2DOUBLE(msg->start().z());

    end.x = FIXED2DOUBLE(msg->end().x());
    end.y = FIXED2DOUBLE(msg->end().y());
    end.z = FIXED2DOUBLE(msg->end().z());

    P_DrawRailTrail(start, end);
}

static void CL_PlayerState(const odaproto::svc::PlayerState *msg)
{
    uint8_t      id          = msg->player().playerid();
    int32_t      health      = msg->player().health();
    int32_t      armortype   = msg->player().armortype();
    int32_t      armorpoints = msg->player().armorpoints();
    int32_t      lives       = msg->player().lives();
    weapontype_t weap        = static_cast<weapontype_t>(msg->player().readyweapon());

    uint8_t        cardByte = msg->player().cards();
    std::bitset<6> cardBits(cardByte);

    int32_t ammo[NUMAMMO];
    for (int32_t i = 0; i < NUMAMMO; i++)
    {
        if (i < msg->player().ammo_size())
        {
            ammo[i] = msg->player().ammo().Get(i);
        }
        else
        {
            ammo[i] = 0;
        }
    }

    statenum_t stnum[NUMPSPRITES] = {S_NULL, S_NULL};
    for (int32_t i = 0; i < NUMPSPRITES; i++)
    {
        if (i < msg->player().psprites_size())
        {
            uint32_t state = msg->player().psprites().Get(i).statenum();
            if (state >= NUMSTATES)
            {
                continue;
            }
            stnum[i] = static_cast<statenum_t>(state);
        }
    }

    int32_t powerups[NUMPOWERS];
    for (int32_t i = 0; i < NUMPOWERS; i++)
    {
        if (i < msg->player().powers_size())
        {
            powerups[i] = msg->player().powers().Get(i);
        }
        else
        {
            powerups[i] = 0;
        }
    }

    uint32_t cheats = msg->player().cheats();

    player_t &player = idplayer(id);
    if (!validplayer(player) || !player.mo)
        return;

    player.health = player.mo->health = health;
    player.armortype                  = armortype;
    player.armorpoints                = armorpoints;
    player.lives                      = lives;

    player.readyweapon   = weap;
    player.pendingweapon = wp_nochange;

    for (int32_t i = 0; i < NUMCARDS; i++)
        player.cards[i] = cardBits[i];

    if (!player.weaponowned[weap])
        P_GiveWeapon(&player, weap, false);

    for (int32_t i = 0; i < NUMAMMO; i++)
        player.ammo[i] = ammo[i];

    for (int32_t i = 0; i < NUMPSPRITES; i++)
        P_SetPsprite(&player, i, stnum[i]);

    for (int32_t i = 0; i < NUMPOWERS; i++)
        player.powers[i] = powerups[i];

    if (!player.spectator)
        player.cheats = cheats;
}

/**
 * @brief Set local levelstate.
 */
static void CL_LevelState(const odaproto::svc::LevelState *msg)
{
    // Set local levelstate.
    SerializedLevelState sls;
    sls.state               = static_cast<LevelState::States>(msg->state());
    sls.countdown_done_time = msg->countdown_done_time();
    sls.ingame_start_time   = msg->ingame_start_time();
    sls.round_number        = msg->round_number();
    sls.last_wininfo_type   = static_cast<WinInfo::WinType>(msg->last_wininfo_type());
    sls.last_wininfo_id     = msg->last_wininfo_id();
    ::levelstate.unserialize(sls);
}

static void CL_ResetMap(const odaproto::svc::ResetMap *msg)
{
    ClientReplay::getInstance().reset();

    // Destroy every actor with a netid that isn't a player.  We're going to
    // get the contents of the map with a full update later on anyway.
    AActor                  *mo;
    TThinkerIterator<AActor> iterator;
    while ((mo = iterator.Next()))
    {
        if (mo->netid && mo->type != MT_PLAYER)
        {
            mo->Destroy();
        }
    }

    // destroy all moving sector effects and sounds
    // Also restore original light levels
    // so light glowing looks normal
    for (int32_t i = 0; i < numsectors; i++)
    {
        if (sectors[i].floordata)
        {
            S_StopSound(sectors[i].soundorg);
            sectors[i].floordata->Destroy();
        }

        if (sectors[i].ceilingdata)
        {
            S_StopSound(sectors[i].soundorg);
            sectors[i].ceilingdata->Destroy();
        }

        // Restore the old light levels so lighting effects look good every time
        sectors[i].lightlevel = ::originalLightLevels[i];
    }

    P_DestroyButtonThinkers();

    P_DestroyScrollerThinkers();

    P_DestroyLightThinkers();

    // You don't get to keep cards.  This isn't communicated anywhere else.
    if (sv_gametype == GM_COOP)
        P_ClearPlayerCards(consoleplayer());
}

static void CL_PlayerQueuePos(const odaproto::svc::PlayerQueuePos *msg)
{
    player_t &player   = idplayer(msg->pid());
    uint8_t   queuePos = msg->queuepos();

    if (player.id == consoleplayer_id)
    {
        if (queuePos > 0 && player.QueuePosition == 0)
        {
            Printf(PRINT_HIGH, "Position in line to play: %u\n", queuePos);
        }
        else if (player.spectator && queuePos == 0 && player.QueuePosition > 0)
        {
            Printf(PRINT_HIGH, "You have been removed from the queue.\n");
        }
    }

    player.QueuePosition = queuePos;
}

static void CL_FullUpdateStart(const odaproto::svc::FullUpdateStart *msg)
{
    ::recv_full_update = false;
}

static void CL_LineUpdate(const odaproto::svc::LineUpdate *msg)
{
    int32_t linenum = msg->linenum();
    int16_t flags   = msg->flags();
    uint8_t lucency = msg->lucency();

    if (linenum < 0 || linenum >= ::numlines)
        return;

    line_t *line  = &lines[linenum];
    line->flags   = flags;
    line->lucency = lucency;
}

/**
 * @brief Update sector properties dynamically.
 */
static void CL_SectorProperties(const odaproto::svc::SectorProperties *msg)
{
    int32_t  secnum  = msg->sectornum();
    uint32_t changes = msg->changes();

    if (secnum < 0 || secnum >= ::numsectors)
        return;

    sector_t *sector = &::sectors[secnum];

    for (int32_t i = 0, prop = 1; prop < SPC_Max; i++)
    {
        prop = 1 << i;
        if ((prop & changes) == 0)
            continue;

        switch (prop)
        {
        case SPC_FlatPic:
            sector->floorpic   = msg->sector().floorpic();
            sector->ceilingpic = msg->sector().ceilingpic();
            break;
        case SPC_LightLevel:
            sector->lightlevel = msg->sector().lightlevel();
            break;
        case SPC_Color: {
            uint8_t r        = msg->sector().colormap().color().r();
            uint8_t g        = msg->sector().colormap().color().g();
            uint8_t b        = msg->sector().colormap().color().b();
            sector->colormap = GetSpecialLights(r, g, b, sector->colormap->fade.getr(), sector->colormap->fade.getg(),
                                                sector->colormap->fade.getb());
            break;
        }
        case SPC_Fade: {
            uint8_t r        = msg->sector().colormap().fade().r();
            uint8_t g        = msg->sector().colormap().fade().g();
            uint8_t b        = msg->sector().colormap().fade().b();
            sector->colormap = GetSpecialLights(sector->colormap->color.getr(), sector->colormap->color.getg(),
                                                sector->colormap->color.getb(), r, g, b);
            break;
        }
        case SPC_Gravity:
            *(int32_t *)&sector->gravity = msg->sector().gravity();
            break;
        case SPC_Panning:
            sector->ceiling_xoffs = msg->sector().ceiling_offs().x();
            sector->ceiling_yoffs = msg->sector().ceiling_offs().y();
            sector->floor_xoffs   = msg->sector().floor_offs().x();
            sector->floor_yoffs   = msg->sector().floor_offs().y();
            break;
        case SPC_Scale:
            sector->ceiling_xscale = msg->sector().ceiling_scale().x();
            sector->ceiling_yscale = msg->sector().ceiling_scale().y();
            sector->floor_xscale   = msg->sector().floor_scale().x();
            sector->floor_yscale   = msg->sector().floor_scale().y();
            break;
        case SPC_Rotation:
            sector->floor_angle   = msg->sector().floor_angle();
            sector->ceiling_angle = msg->sector().ceiling_angle();
            break;
        case SPC_AlignBase:
            sector->base_ceiling_angle = msg->sector().base_ceiling_angle();
            sector->base_ceiling_yoffs = msg->sector().base_ceiling_yoffs();
            sector->base_floor_angle   = msg->sector().base_floor_angle();
            sector->base_floor_yoffs   = msg->sector().base_floor_yoffs();
        default:
            break;
        }
    }
}

static void CL_LineSideUpdate(const odaproto::svc::LineSideUpdate *msg)
{
    int32_t  linenum = msg->linenum();
    int32_t  side    = msg->side();
    uint32_t changes = msg->changes();

    if (linenum < 0 || linenum >= ::numlines)
        return;

    if (side < 0 || side >= 2 || lines[linenum].sidenum[side] != R_NOSIDE)
        return;

    side_t *currentSidedef = sides + lines[linenum].sidenum[side];

    for (int32_t i = 0, prop = 1; prop < SDPC_Max; i++)
    {
        prop = BIT(i);
        if ((prop & changes) == 0)
            continue;

        switch (prop)
        {
        case SDPC_TexTop:
            currentSidedef->toptexture = msg->toptexture();
            break;
        case SDPC_TexMid:
            currentSidedef->midtexture = msg->midtexture();
            break;
        case SDPC_TexBottom:
            currentSidedef->bottomtexture = msg->bottomtexture();
            break;
        default:
            break;
        }
    }
}

//
// CL_SetMobjState
//
static void CL_SetMobjState(const odaproto::svc::MobjState *msg)
{
    AActor *mo = P_FindThingById(msg->netid());
    int32_t s  = msg->mostate();

    if (mo == NULL || s < 0 || s >= NUMSTATES)
        return;

    P_SetMobjState(mo, static_cast<statenum_t>(s));
}

//
// CL_DamageMobj
//
static void CL_DamageMobj(const odaproto::svc::DamageMobj *msg)
{
    uint32_t netid  = msg->netid();
    int32_t  health = msg->health();
    int32_t  pain   = msg->pain();

    AActor *mo = P_FindThingById(netid);

    if (mo == NULL)
        return;

    mo->health = health;

    if (pain < mo->info->painchance)
        P_SetMobjState(mo, mo->info->painstate);
}

static void CL_ExecuteLineSpecial(const odaproto::svc::ExecuteLineSpecial *msg)
{
    uint8_t special   = msg->special();
    int32_t linenum   = msg->linenum();
    AActor *activator = P_FindThingById(msg->activator_netid());
    int32_t arg0      = msg->arg0();
    int32_t arg1      = msg->arg1();
    int32_t arg2      = msg->arg2();
    int32_t arg3      = msg->arg3();
    int32_t arg4      = msg->arg4();

    if (linenum != -1 && linenum >= ::numlines)
        return;

    line_s *line = NULL;
    if (linenum != -1)
        line = &::lines[linenum];

    ActivateLine(activator, line, 0, LineACS, false, special, arg0, arg1, arg2, arg3, arg4);
}

static void CL_ExecuteACSSpecial(const odaproto::svc::ExecuteACSSpecial *msg)
{
    uint8_t     special = msg->special();
    uint32_t    netid   = msg->activator_netid();
    std::string print   = msg->print();
    uint8_t     count   = msg->args().size();

    int32_t acsArgs[16];
    ArrayInit(acsArgs, 0);
    std::copy(msg->args().begin(), msg->args().end(), acsArgs);

    AActor *activator = P_FindThingById(netid);

    switch (special)
    {
    case DLevelScript::PCD_CLEARINVENTORY:
        DLevelScript::ACS_ClearInventory(activator);
        break;

    case DLevelScript::PCD_SETLINETEXTURE:
        DLevelScript::ACS_SetLineTexture(acsArgs, count);
        break;

    case DLevelScript::PCD_ENDPRINT:
    case DLevelScript::PCD_ENDPRINTBOLD:
        DLevelScript::ACS_Print(special, activator, print.c_str());
        break;

    case DLevelScript::PCD_SETMUSIC:
    case DLevelScript::PCD_SETMUSICDIRECT:
    case DLevelScript::PCD_LOCALSETMUSIC:
    case DLevelScript::PCD_LOCALSETMUSICDIRECT:
        DLevelScript::ACS_ChangeMusic(special, activator, acsArgs, count);
        break;

    case DLevelScript::PCD_SECTORSOUND:
    case DLevelScript::PCD_AMBIENTSOUND:
    case DLevelScript::PCD_LOCALAMBIENTSOUND:
    case DLevelScript::PCD_ACTIVATORSOUND:
    case DLevelScript::PCD_THINGSOUND:
        DLevelScript::ACS_StartSound(special, activator, acsArgs, count);
        break;

    case DLevelScript::PCD_SETLINEBLOCKING:
        DLevelScript::ACS_SetLineBlocking(acsArgs, count);
        break;

    case DLevelScript::PCD_SETLINEMONSTERBLOCKING:
        DLevelScript::ACS_SetLineMonsterBlocking(acsArgs, count);
        break;

    case DLevelScript::PCD_SETLINESPECIAL:
        DLevelScript::ACS_SetLineSpecial(acsArgs, count);
        break;

    case DLevelScript::PCD_SETTHINGSPECIAL:
        DLevelScript::ACS_SetThingSpecial(acsArgs, count);
        break;

    case DLevelScript::PCD_FADERANGE:
        DLevelScript::ACS_FadeRange(activator, acsArgs, count);
        break;

    case DLevelScript::PCD_CANCELFADE:
        DLevelScript::ACS_CancelFade(activator);
        break;

    case DLevelScript::PCD_CHANGEFLOOR:
    case DLevelScript::PCD_CHANGECEILING:
        DLevelScript::ACS_ChangeFlat(special, acsArgs, count);
        break;

    case DLevelScript::PCD_SOUNDSEQUENCE:
        DLevelScript::ACS_SoundSequence(acsArgs, count);
        break;

    default:
        Printf(PRINT_HIGH, "Invalid ACS special: %d", special);
        break;
    }
}

/**
 * @brief Update a thinker.
 */
static void CL_ThinkerUpdate(const odaproto::svc::ThinkerUpdate *msg)
{
    switch (msg->thinker_case())
    {
    case odaproto::svc::ThinkerUpdate::kScroller: {
        DScroller::EScrollType scrollType = static_cast<DScroller::EScrollType>(msg->scroller().type());
        fixed_t                dx         = msg->scroller().scroll_x();
        fixed_t                dy         = msg->scroller().scroll_y();
        int32_t                affectee   = msg->scroller().affectee();
        int32_t                accel      = msg->scroller().accel();
        int32_t                control    = msg->scroller().control();
        if (::numsides <= 0 || ::numsectors <= 0)
            break;
        if (affectee < 0)
            break;
        if (scrollType == DScroller::sc_side && affectee > ::numsides)
            break;
        if (scrollType != DScroller::sc_side && affectee > ::numsectors)
            break;
        // remove null checks after 11 is released
        // because right now, control sectors of 0 won't scroll
        if (!control || control < 0)
            control = -1;
        if (!accel || accel < 0)
            accel = 0;

        new DScroller(scrollType, dx, dy, control, affectee, accel);
        break;
    }
    case odaproto::svc::ThinkerUpdate::kFireFlicker: {
        int16_t secnum = msg->fire_flicker().sector();
        int32_t min    = msg->fire_flicker().min_light();
        int32_t max    = msg->fire_flicker().max_light();
        if (::numsectors <= 0)
            break;
        if (secnum < ::numsectors)
            new DFireFlicker(&::sectors[secnum], max, min);
        break;
    }
    case odaproto::svc::ThinkerUpdate::kFlicker: {
        int16_t secnum = msg->flicker().sector();
        int32_t min    = msg->flicker().min_light();
        int32_t max    = msg->flicker().max_light();
        if (::numsectors <= 0)
            break;
        if (secnum < ::numsectors)
            new DFlicker(&::sectors[secnum], max, min);
        break;
    }
    case odaproto::svc::ThinkerUpdate::kLightFlash: {
        int16_t secnum = msg->light_flash().sector();
        int32_t min    = msg->light_flash().min_light();
        int32_t max    = msg->light_flash().max_light();
        if (::numsectors <= 0)
            break;
        if (secnum < ::numsectors)
            new DLightFlash(&::sectors[secnum], min, max);
        break;
    }
    case odaproto::svc::ThinkerUpdate::kStrobe: {
        int16_t secnum = msg->strobe().sector();
        int32_t min    = msg->strobe().min_light();
        int32_t max    = msg->strobe().max_light();
        int32_t dark   = msg->strobe().dark_time();
        int32_t bright = msg->strobe().bright_time();
        int32_t count  = msg->strobe().count();
        if (::numsectors <= 0)
            break;
        if (secnum < ::numsectors)
        {
            DStrobe *strobe = new DStrobe(&::sectors[secnum], max, min, bright, dark);
            strobe->SetCount(count);
        }
        break;
    }
    case odaproto::svc::ThinkerUpdate::kGlow: {
        int16_t secnum = msg->glow().sector();
        if (::numsectors <= 0)
            break;
        if (secnum < ::numsectors)
            new DGlow(&::sectors[secnum]);
        break;
    }
    case odaproto::svc::ThinkerUpdate::kGlow2: {
        int16_t secnum  = msg->glow2().sector();
        int32_t start   = msg->glow2().start();
        int32_t end     = msg->glow2().end();
        int32_t tics    = msg->glow2().max_tics();
        bool    oneShot = msg->glow2().one_shot();
        if (::numsectors <= 0)
            break;
        if (secnum < ::numsectors)
            new DGlow2(&::sectors[secnum], start, end, tics, oneShot);
        break;
    }
    case odaproto::svc::ThinkerUpdate::kPhased: {
        int16_t secnum = msg->phased().sector();
        int32_t base   = msg->phased().base_level();
        int32_t phase  = msg->phased().phase();
        if (::numsectors <= 0)
            break;
        if (secnum < ::numsectors)
            new DPhased(&::sectors[secnum], base, phase);
        break;
    }
    default:
        break;
    }
}

static void CL_VoteUpdate(const odaproto::svc::VoteUpdate *msg)
{
    vote_result_t result = static_cast<vote_result_t>(msg->result());

    if (result < 0 || result >= NUMVOTERESULTS)
        return;

    vote_state_t vote_state;
    vote_state.result     = result;
    vote_state.votestring = msg->votestring();
    vote_state.countdown  = msg->countdown();
    vote_state.yes        = msg->yes();
    vote_state.yes_needed = msg->yes_needed();
    vote_state.no         = msg->no();
    vote_state.no_needed  = msg->no_needed();
    vote_state.abs        = msg->abs();

    VoteState::instance().set(vote_state);
}

// Got a packet that contains the maplist status
static void CL_Maplist(const odaproto::svc::Maplist *msg)
{
    // The update status might require us to bail out.
    maplist_status_t status = static_cast<maplist_status_t>(msg->status());
    if (status < 0 || status >= NUM_MAPLIST_STATUS)
        return;

    MaplistCache::instance().status_handler(status);
}

// Got a packet that contains a chunk of the maplist.
static void CL_MaplistUpdate(const odaproto::svc::MaplistUpdate *msg)
{
    // The update status might require us to bail out.
    maplist_status_t status = static_cast<maplist_status_t>(msg->status());
    if (status < 0 || status >= NUM_MAPLIST_STATUS)
        return;

    // Some statuses require an early out.
    if (!MaplistCache::instance().update_status_handler(status))
        return;

    OStringIndexer indexer = OStringIndexer::maplistFactory();

    // Parse our dictionary first.
    google::protobuf::Map<uint32_t, std::string>::const_iterator it;
    for (it = msg->dict().begin(); it != msg->dict().end(); ++it)
    {
        indexer.setIndex(it->first, it->second);
    }

    // Load our maps into the local cache.
    MaplistCache::instance().set_size(msg->maplist().size());

    for (int32_t i = 0; i < msg->maplist().size(); i++)
    {
        const odaproto::svc::MaplistUpdate::Row &row = msg->maplist().Get(i);
        const std::string                       &map = indexer.getString(row.map());

        maplist_entry_t maplist_entry;
        maplist_entry.map = map;
        for (int32_t j = 0; j < row.wads_size(); j++)
        {
            const std::string &wad = indexer.getString(row.wads().Get(j));
            maplist_entry.wads.push_back(wad);
        }

        MaplistCache::instance().set_cache_entry(i, maplist_entry);
    }
}

// Got a packet that contains the next and current index.
static void CL_MaplistIndex(const odaproto::svc::MaplistIndex *msg)
{
    if (msg->count() > 0)
    {
        MaplistCache::instance().set_next_index(msg->next_index());
        if (msg->count() > 1)
        {
            MaplistCache::instance().set_this_index(msg->this_index());
        }
        else
        {
            MaplistCache::instance().unset_this_index();
        }
    }
}

static void CL_Toast(const odaproto::svc::Toast *msg)
{
    /*
    toast_t toast;
    toast.flags     = msg->flags();
    toast.left      = msg->left();
    toast.left_pid  = msg->left_pid();
    toast.right     = msg->right();
    toast.right_pid = msg->right_pid();
    toast.icon      = msg->icon();

    COM_PushToast(toast);
    */
}

//-----------------------------------------------------------------------------
// Everything below this line is not a message parsing funciton.
//-----------------------------------------------------------------------------

Protos protos;

static void RecordProto(const svc_t header, google::protobuf::Message *msg)
{
    static int32_t protostic;

    if (protostic != ::level.time)
    {
        ::protos.clear();
        protostic = ::level.time;
    }

    Proto proto;
    proto.header = header;
    proto.name   = ::svc_info[(uint8_t)header].getName();
    if (msg)
    {
        proto.size = msg->ByteSizeLong();
        proto.data = msg->DebugString();

        // Replace braces in debug string - we don't have that char in the font.
        for (size_t i = 0; i < proto.data.size(); i++)
        {
            if (proto.data[i] == '{')
                proto.data[i] = '(';
            else if (proto.data[i] == '}')
                proto.data[i] = ')';
        }
        TrimStringEnd(proto.data);
    }
    ::protos.push_back(proto);
}

const Protos &CL_GetTicProtos()
{
    return ::protos;
}

/**
 * @brief Given a message type and buffer, return a decoded message in "out".
 *
 * @param out Output message - will not be modified unless successful.
 * @param cmd Command to parse out.
 * @param buffer Buffer to parse, not including the header or initial size.
 * @param size Length of the buffer to parse.
 * @return Error condition, or OK (0) if successful.
 */
parseError_e CL_ParseMessage(google::protobuf::Message *&out, const uint8_t cmd, const void *buffer, const size_t size)
{
    // A message factory + Descriptor gives us the proper message.
    google::protobuf::MessageFactory   *factory = google::protobuf::MessageFactory::generated_factory();
    const google::protobuf::Descriptor *desc    = SVC_ResolveHeader(static_cast<svc_t>(cmd));
    if (desc == NULL)
    {
        return PERR_UNKNOWN_HEADER;
    }

    // Can we get the mssage prototype from the descriptor?
    const google::protobuf::Message *defmsg = factory->GetPrototype(desc);
    if (defmsg == NULL)
    {
        return PERR_UNKNOWN_MESSAGE;
    }

    // Allocated with "new" - can't be null, and we own it.
    google::protobuf::Message *msg = defmsg->New();
    if (!msg->ParseFromArray(buffer, size))
    {
        return PERR_BAD_DECODE;
    }

    out = msg;
    return PERR_OK;
}

#define SV_MSG(header, func, type)                                                                                     \
    case header:                                                                                                       \
        func(static_cast<const type *>(msg));                                                                          \
        break

/**
 * @brief Read a server message off the wire.
 */
parseError_e CL_ParseCommand()
{
    // What type of message we have.
    uint8_t cmd = MSG_ReadByte();

    // Size of the message.
    size_t size = MSG_ReadUnVarint();

    // The message itself.
    void *data = MSG_ReadChunk(size);

    // Turn the message into a protobuf.
    google::protobuf::Message *msg = NULL;
    parseError_e               err = CL_ParseMessage(msg, cmd, data, size);
    if (err)
    {
        return err;
    }

    // Delete pointer on scope exit.
    std::unique_ptr<google::protobuf::Message> autoMSG(msg);

    // Run the proper message function.
    switch (cmd)
    {
        /* clang-format off */
		SV_MSG(svc_noop, CL_Noop, odaproto::svc::Noop);
		SV_MSG(svc_disconnect, CL_Disconnect, odaproto::svc::Disconnect);
		SV_MSG(svc_playerinfo, CL_PlayerInfo, odaproto::svc::PlayerInfo);
		SV_MSG(svc_moveplayer, CL_MovePlayer, odaproto::svc::MovePlayer);
		SV_MSG(svc_updatelocalplayer, CL_UpdateLocalPlayer, odaproto::svc::UpdateLocalPlayer);
		SV_MSG(svc_levellocals, CL_LevelLocals, odaproto::svc::LevelLocals);
		SV_MSG(svc_pingrequest, CL_PingRequest, odaproto::svc::PingRequest);
		SV_MSG(svc_updateping, CL_UpdatePing, odaproto::svc::UpdatePing);
		SV_MSG(svc_spawnmobj, CL_SpawnMobj, odaproto::svc::SpawnMobj);
		SV_MSG(svc_disconnectclient, CL_DisconnectClient, odaproto::svc::DisconnectClient);
		SV_MSG(svc_loadmap, CL_LoadMap, odaproto::svc::LoadMap);
		SV_MSG(svc_consoleplayer, CL_ConsolePlayer, odaproto::svc::ConsolePlayer);
		SV_MSG(svc_explodemissile, CL_ExplodeMissile, odaproto::svc::ExplodeMissile);
		SV_MSG(svc_removemobj, CL_RemoveMobj, odaproto::svc::RemoveMobj);
		SV_MSG(svc_userinfo, CL_UserInfo, odaproto::svc::UserInfo);
		SV_MSG(svc_updatemobj, CL_UpdateMobj, odaproto::svc::UpdateMobj);
		SV_MSG(svc_spawnplayer, CL_SpawnPlayer, odaproto::svc::SpawnPlayer);
		SV_MSG(svc_damageplayer, CL_DamagePlayer, odaproto::svc::DamagePlayer);
		SV_MSG(svc_killmobj, CL_KillMobj, odaproto::svc::KillMobj);
		SV_MSG(svc_fireweapon, CL_FireWeapon, odaproto::svc::FireWeapon);
		SV_MSG(svc_updatesector, CL_UpdateSector, odaproto::svc::UpdateSector);
		SV_MSG(svc_print, CL_Print, odaproto::svc::Print);
		SV_MSG(svc_playermembers, CL_PlayerMembers, odaproto::svc::PlayerMembers);
		SV_MSG(svc_teammembers, CL_TeamMembers, odaproto::svc::TeamMembers);
		SV_MSG(svc_activateline, CL_ActivateLine, odaproto::svc::ActivateLine);
		SV_MSG(svc_movingsector, CL_MovingSector, odaproto::svc::MovingSector);
		SV_MSG(svc_playsound, CL_PlaySound, odaproto::svc::PlaySound);
		SV_MSG(svc_reconnect, CL_Reconnect, odaproto::svc::Reconnect);
		SV_MSG(svc_exitlevel, CL_ExitLevel, odaproto::svc::ExitLevel);
		SV_MSG(svc_touchspecial, CL_TouchSpecial, odaproto::svc::TouchSpecial);
		SV_MSG(svc_forceteam, CL_ForceTeam, odaproto::svc::ForceTeam);
		SV_MSG(svc_switch, CL_Switch, odaproto::svc::Switch);
		SV_MSG(svc_say, CL_Say, odaproto::svc::Say);
		SV_MSG(svc_secretevent, CL_SecretEvent, odaproto::svc::SecretEvent);
		SV_MSG(svc_serversettings, CL_ServerSettings, odaproto::svc::ServerSettings);
		SV_MSG(svc_connectclient, CL_ConnectClient, odaproto::svc::ConnectClient);
		SV_MSG(svc_midprint, CL_MidPrint, odaproto::svc::MidPrint);
		SV_MSG(svc_servergametic, CL_ServerGametic, odaproto::svc::ServerGametic);
		SV_MSG(svc_inttimeleft, CL_IntTimeLeft, odaproto::svc::IntTimeLeft);
		SV_MSG(svc_fullupdatedone, CL_FullUpdateDone, odaproto::svc::FullUpdateDone);
		SV_MSG(svc_railtrail, CL_RailTrail, odaproto::svc::RailTrail);
		SV_MSG(svc_playerstate, CL_PlayerState, odaproto::svc::PlayerState);
		SV_MSG(svc_levelstate, CL_LevelState, odaproto::svc::LevelState);
		SV_MSG(svc_resetmap, CL_ResetMap, odaproto::svc::ResetMap);
		SV_MSG(svc_playerqueuepos, CL_PlayerQueuePos, odaproto::svc::PlayerQueuePos);
		SV_MSG(svc_fullupdatestart, CL_FullUpdateStart, odaproto::svc::FullUpdateStart);
		SV_MSG(svc_lineupdate, CL_LineUpdate, odaproto::svc::LineUpdate);
		SV_MSG(svc_sectorproperties, CL_SectorProperties, odaproto::svc::SectorProperties);
		SV_MSG(svc_linesideupdate, CL_LineSideUpdate, odaproto::svc::LineSideUpdate);
		SV_MSG(svc_mobjstate, CL_SetMobjState, odaproto::svc::MobjState);
		SV_MSG(svc_damagemobj, CL_DamageMobj, odaproto::svc::DamageMobj);
		SV_MSG(svc_executelinespecial, CL_ExecuteLineSpecial, odaproto::svc::ExecuteLineSpecial);
		SV_MSG(svc_executeacsspecial, CL_ExecuteACSSpecial, odaproto::svc::ExecuteACSSpecial);
		SV_MSG(svc_thinkerupdate, CL_ThinkerUpdate, odaproto::svc::ThinkerUpdate);
		SV_MSG(svc_vote_update, CL_VoteUpdate, odaproto::svc::VoteUpdate);
		SV_MSG(svc_maplist, CL_Maplist, odaproto::svc::Maplist);
		SV_MSG(svc_maplist_update, CL_MaplistUpdate, odaproto::svc::MaplistUpdate);
		SV_MSG(svc_maplist_index, CL_MaplistIndex, odaproto::svc::MaplistIndex);
		SV_MSG(svc_toast, CL_Toast, odaproto::svc::Toast);
        /* clang-format on */
    default:
        return PERR_UNKNOWN_HEADER;
    }

    RecordProto(static_cast<svc_t>(cmd), msg);
    return PERR_OK;
}
