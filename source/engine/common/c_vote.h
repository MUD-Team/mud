// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 9b377e7d26c961bf409726ca0ffd8ad55911c84b $
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
//	Voting-specific stuff.
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>

#include "doomtype.h"

/**
 * An enum used for keeping track of the state of the vote as well as
 * internally keeping track of individual player votes.
 */
enum vote_result_t
{
    VOTE_UNDEC,
    VOTE_NO,
    VOTE_YES,
    VOTE_INTERRUPT,
    VOTE_ABANDON
};

#define NUMVOTERESULTS (VOTE_ABANDON + 1)

/**
 * An enum used for sending the vote type over the wire.
 */
enum vote_type_t
{
    VOTE_NONE, // Reserved
    VOTE_KICK,
    VOTE_FORCESPEC,
    VOTE_FORCESTART,
    VOTE_RANDCAPS,
    VOTE_RANDPICKUP,
    VOTE_MAP,
    VOTE_NEXTMAP,
    VOTE_RANDMAP,
    VOTE_RESTART,
    VOTE_FRAGLIMIT,
    VOTE_SCORELIMIT,
    VOTE_TIMELIMIT,
    VOTE_COINFLIP,
    VOTE_MAX // Reserved
};

// A struct to pass around voting state
struct vote_state_t
{
    vote_result_t result;
    std::string   votestring;
    int16_t       countdown;
    uint8_t       yes;
    uint8_t       yes_needed;
    uint8_t       no;
    uint8_t       no_needed;
    uint8_t       abs;
};

extern const char *vote_type_cmd[];
