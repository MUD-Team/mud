// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d4fa1beab05eb05636fe1970a08fe9b95cb9a52e $
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

#include "stats.h"

#include "c_dispatch.h"
#include "i_system.h"
#include "mud_includes.h"
#include "v_video.h"

std::vector<FStat *> FStat::stats;

FStat::FStat(const char *cname) : name(cname)
{
    stats.push_back(this);
}

FStat::~FStat()
{
    std::vector<FStat *>::iterator i = std::find(stats.begin(), stats.end(), this);

    if (i != stats.end())
        stats.erase(i);
}

void FStat::clock()
{
    last_clock = I_MSTime();
}

void FStat::unclock()
{
    last_elapsed = I_MSTime() - last_clock;
}

void FStat::reset()
{
    last_elapsed = last_clock = 0;
}

const char *FStat::getname()
{
    return name.c_str();
}

void FStat::dumpstat()
{
    for (size_t i = 0; i < stats.size(); i++)
        Printf(PRINT_HIGH, "%s\n", stats[i]->getname());
}

void FStat::dumpstat(std::string which)
{
    for (size_t i = 0; i < stats.size(); i++)
        if (which == stats[i]->name)
            stats[i]->dump();
}

void FStat::dump()
{
    Printf(PRINT_HIGH, "%s: %dms\n", name.c_str(), last_elapsed);
}

BEGIN_COMMAND(stat)
{
    if (argc != 2)
    {
        Printf(PRINT_HIGH, "Usage: stat <statistics>\n");
        FStat::dumpstat();
    }
    else
    {
        FStat::dumpstat(argv[1]);
    }
}
END_COMMAND(stat)

VERSION_CONTROL(stats_cpp, "$Id: d4fa1beab05eb05636fe1970a08fe9b95cb9a52e $")
