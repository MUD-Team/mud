// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 902cba50121edb6e0668bffa2b52c31ba48bdbd1 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2022 by The Odamex Team.
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
//   [Blair] This system is used to replay certain messages on the client
//   when messages arrive before the object they reference does.
//   This is usually due to high load, lag, or initial map reset
//   which heavily throttles the initial item send.
//
//-----------------------------------------------------------------------------

#pragma once

#include <map>

#include "actor.h"
#include "d_player.h"
#include "m_fixed.h"
#include "r_defs.h"

class ClientReplay
{
  public:
    ~ClientReplay();
    static ClientReplay &getInstance(); // returns the instantiated ClientReplay object
    void                 reset();       // called when starting/resetting a level
    static bool          enabled();
    void                 recordReplayItem(const int32_t tic, const uint32_t netId);
    void                 removeReplayItem(const std::pair<int32_t, uint32_t> replayItem);
    void                 itemReplay();
    bool                 wasReplayed();

  private:
    std::vector<std::pair<int32_t, uint32_t>>
                          itemReplayStack; // Used to replay item pickups for items the clients can't find.
    static const uint32_t MAX_REPLAY_TIC_LENGTH = TICRATE * 3; // Should be plenty of time.
    bool                  replayed              = false;
    uint32_t              replayDoneCounter     = TICRATE * 7;
    uint32_t              firstReadyTic         = 0;
    // <int32_t, uint32_t> = <gametic, itemid>
    ClientReplay()
    {
    } // private contsructor (part of Singleton)
    ClientReplay(const ClientReplay &rhs); // private copy constructor
};
