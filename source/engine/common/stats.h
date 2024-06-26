// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 415ca654317dcb8a011c8d702cf59db6e552f6e4 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	STATS
//
//-----------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "doomtype.h"

class FStat
{
  public:
    FStat(const char *cname);

    virtual ~FStat();

    void clock();
    void unclock();
    void reset();

    const char *getname();

    static void dumpstat();
    static void dumpstat(std::string which);
    void        dump();

  private:
    uint64_t                    last_clock, last_elapsed;
    std::string                 name;
    static std::vector<FStat *> stats;
};

#define BEGIN_STAT(n)                                                                                                  \
    static class Stat_##n : public FStat                                                                               \
    {                                                                                                                  \
      public:                                                                                                          \
        Stat_##n() : FStat(#n)                                                                                         \
        {                                                                                                              \
        }                                                                                                              \
    } Stat_var_##n;                                                                                                    \
    Stat_var_##n.clock();

#define END_STAT(n) Stat_var_##n.unclock();
