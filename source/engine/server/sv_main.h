// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: cdf8d4c119b39ee07a379a6892e1783c7a188adf $
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
//	SV_MAIN
//
//-----------------------------------------------------------------------------

#pragma once

#include "actor.h"
#include "d_player.h"
#include "g_gametype.h"
#include "i_net.h"

extern bool keysfound[NUMCARDS];

class client_c
{
  public:
    size_t size()
    {
        return players.size();
    }
    client_c()
    {
    }
};

extern client_c clients;

void SV_InitNetwork(void);
void SV_SendDisconnectSignal();
void SV_SendReconnectSignal();
void SV_ExitLevel();
void SV_DrawScores();

void SV_ServerSettingChange();
bool SV_IsPlayerAllowedToSee(player_t &pl, AActor *mobj);

void STACK_ARGS SV_ClientPrintf(client_t *cl, int32_t level, const char *fmt, ...);
void STACK_ARGS SV_SpectatorPrintf(int32_t level, const char *fmt, ...);
void STACK_ARGS SV_PlayerPrintf(int32_t level, int32_t who, const char *fmt, ...);
void            SV_CheckTimeouts(void);
void            SV_ConnectClient(void);
void            SV_ConnectClient2(player_t &player);
void            SV_WriteCommands(void);
void            SV_ClearClientsBPS(void);
bool            SV_SendPacket(player_t &pl);
void            SV_AcknowledgePacket(player_t &player);
void            SV_DisplayTics();
void            SV_RunTics();
void            SV_ParseCommands(player_t &player);
void            SV_UpdateFrags(player_t &player);
void            SV_RemoveCorpses(void);
#define SV_DropClient(who) SV_DropClient2(who, __FILE__, __LINE__)
void SV_DropClient2(player_t &who, const char *file, const int32_t line);
void SV_PlayerTriedToCheat(player_t &player);
void SV_ActorTarget(AActor *actor);
void SV_ActorTracer(AActor *actor);
void SV_ForceSetTeam(player_t &who, team_t team);
void SV_CheckTeam(player_t &player);
void SV_SendUserInfo(player_t &player, client_t *cl);
void SV_Suicide(player_t &player);
void SV_SpawnMobj(AActor *mo);
void SV_TouchSpecial(AActor *special, player_t *player);

void SV_Sound(AActor *mo, uint8_t channel, const char *name, uint8_t attenuation);
void SV_Sound(player_t &pl, AActor *mo, const uint8_t channel, const char *name, const uint8_t attenuation);
void SV_Sound(fixed_t x, fixed_t y, uint8_t channel, const char *name, uint8_t attenuation);
void SV_SoundTeam(uint8_t channel, const char *name, uint8_t attenuation, int32_t t);

void SV_MidPrint(const char *msg, player_t *p, int32_t msgtime = 0);

extern std::vector<std::string> wadnames;

void SV_SendPlayerInfo(player_t &player);
void SV_SendKillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill);
void SV_SendDamagePlayer(player_t *player, AActor *inflictor, int32_t healthDamage, int32_t armorDamage);
void SV_SendDamageMobj(AActor *target, int32_t pain);
// Tells clients to remove an actor from the world as it doesn't exist anymore
void SV_SendDestroyActor(AActor *mo);

// [AM] Coinflip
void CMD_CoinFlip(std::string &result);

// [AM] Spectating and Kicking
bool CMD_KickCheck(std::vector<std::string> arguments, std::string &error, size_t &pid, std::string &reason);
void SV_KickPlayer(player_t &player, const std::string &reason = "");
bool CMD_ForcespecCheck(const std::vector<std::string> &arguments, std::string &error, size_t &pid);
void SV_SetPlayerSpec(player_t &player, bool setting, bool silent = false);
void SV_JoinPlayer(player_t &player, bool silent);
void SV_SpecPlayer(player_t &player, bool silent);
void SV_SetReady(player_t &player, bool setting, bool silent = false);

void SV_AddPlayerToQueue(player_t *player);
void SV_RemovePlayerFromQueue(player_t *player);
void SV_UpdatePlayerQueueLevelChange(const WinInfo &win);
void SV_UpdatePlayerQueuePositions(JoinTest joinTest, player_t *disconnectPlayer);
void SV_SendPlayerQueuePositions(player_t *dest, bool initConnect);
void SV_SendPlayerQueuePosition(player_t *source, player_t *dest);
void SV_ClearPlayerQueue();

void SV_UpdateSecretCount(player_t &player);
void SV_UpdateMonsterRespawnCount();
void SV_SendExecuteLineSpecial(uint8_t special, line_t *line, AActor *activator, int32_t arg0, int32_t arg1,
                               int32_t arg2, int32_t arg3, int32_t arg4);
void SV_ACSExecuteSpecial(uint8_t special, AActor *activator, const char *print, bool playerOnly,
                          const std::vector<int32_t> &args = std::vector<int32_t>());

bool CompareQueuePosition(const player_t *p1, const player_t *p2);

extern bool unnatural_level_progression;
