// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: afa145c32bee161f9f9e69c2f3bc7e346ccff96d $
//
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
//   Manage state for warmup and complicated gametype flows.
//
//-----------------------------------------------------------------------------

#pragma once

#include "d_player.h"

struct SerializedLevelState;

struct WinInfo
{
    enum WinType
    {
        WIN_UNKNOWN,   // Not sure what happened here.
        WIN_NOBODY,    // Everybody lost the game (probably PvE).
        WIN_EVERYBODY, // Everybody won the game (probably PvE).
        WIN_DRAW,      // Tie at the end of the game.
        WIN_PLAYER,    // A single player won the game.
        WIN_TEAM       // A team won the game.
    };

    WinType type;
    int32_t id;

    WinInfo() : type(WIN_UNKNOWN), id(0)
    {
    }
    void reset()
    {
        this->type = WIN_UNKNOWN;
        this->id   = 0;
    }
};

class LevelState
{
  public:
    typedef void (*SetStateCB)(SerializedLevelState);
    enum States
    {
        UNKNOWN,                 // Unknown state.
        WARMUP,                  // Warmup state.
        WARMUP_COUNTDOWN,        // Warmup countdown.
        WARMUP_FORCED_COUNTDOWN, // Forced countdown, can't be cancelled by unreadying.
        PREROUND_COUNTDOWN,      // Before-the-round countdown.
        INGAME,                  // In the middle of a game/round.
        ENDROUND_COUNTDOWN,      // Round complete, a slight pause before the next round.
        ENDGAME_COUNTDOWN,       // Game complete, a slight pause before intermission.
    };
    LevelState()
        : m_state(LevelState::UNKNOWN), m_countdownDoneTime(0), m_ingameStartTime(0), m_roundNumber(0),
          m_setStateCB(NULL)
    {
    }
    int32_t              getCountdown() const;
    team_t               getDefendingTeam() const;
    int32_t              getIngameStartTime() const;
    int32_t              getJoinTimeLeft() const;
    int32_t              getRound() const;
    LevelState::States   getState() const;
    const char          *getStateString() const;
    WinInfo              getWinInfo() const;
    void                 setStateCB(LevelState::SetStateCB cb);
    void                 setWinner(WinInfo::WinType type, int32_t id);
    void                 reset();
    void                 restart();
    void                 forceStart();
    void                 readyToggle();
    void                 endRound();
    void                 endGame();
    void                 tic();
    SerializedLevelState serialize() const;
    void                 unserialize(SerializedLevelState serialized);

  private:
    LevelState::States     m_state;
    int32_t                m_countdownDoneTime;
    int32_t                m_ingameStartTime;
    int32_t                m_roundNumber;
    WinInfo                m_lastWininfo;
    LevelState::SetStateCB m_setStateCB;

    static LevelState::States getStartOfRoundState();
    void                      setState(LevelState::States new_state);
    void                      printRoundStart() const;
};

struct SerializedLevelState
{
    LevelState::States state;
    int32_t            countdown_done_time;
    int32_t            ingame_start_time;
    int32_t            round_number;
    WinInfo::WinType   last_wininfo_type;
    int32_t            last_wininfo_id;
};

extern LevelState levelstate;
