// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 5a342fe49485a1d9bfe7c992f4363c895894f9b3 $
//
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Clientside voting-specific stuff.
//
//-----------------------------------------------------------------------------

#pragma once

#include "c_vote.h"

// A class to actually deal with keeping track of voting state and ensuring
// that the proper variables are set.
class VoteState
{
  private:
    bool          visible;
    vote_result_t result;
    std::string   votestring;
    int16_t         countdown;
    uint64_t         countdown_ms;
    uint8_t          yes;
    uint8_t          yes_needed;
    uint8_t          no;
    uint8_t          no_needed;
    uint8_t          abs;

  public:
    VoteState() : visible(false){};
    static VoteState &instance(void);
    void              set(const vote_state_t &vote_state);
    bool              get(vote_state_t &vote_state);
};
