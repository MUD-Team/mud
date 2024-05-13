// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: b535505383e1d0efa666741873d9ff0e7509a808 $
//
// Copyright (C) 2021 by Alex Mayfield.
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
//  A class that "indexes" a string with a unique ID  Mostly used to compress
//  text tokens that are often repeated.
//
//-----------------------------------------------------------------------------

#include "m_strindex.h"

#include "cmdlib.h"
#include "mud_includes.h"

/**
 * @brief Create indexer with default dictionary containing common maplist
 *        tokens.
 */
OStringIndexer OStringIndexer::maplistFactory()
{
    std::string    buf;
    OStringIndexer stridx;

    // The big two IWADs.
    const char *IWADS[] = {"DOOM.WAD", "DOOM2.WAD"};
    for (size_t i = 0; i < ARRAY_LENGTH(IWADS); i++)
    {
        stridx.getIndex(IWADS[i]);
    }

    // 36 for Ultimate DOOM.
    for (int32_t e = 1; e <= 4; e++)
    {
        for (int32_t m = 1; m <= 9; m++)
        {
            StrFormat(buf, "E%dM%d", e, m);
            stridx.getIndex(buf);
        }
    }

    // 32 for DOOM II/Final Doom.
    for (int32_t i = 1; i <= 32; i++)
    {
        StrFormat(buf, "MAP%02d", i);
        stridx.getIndex(buf);
    }

    // Any other string/index combinations are up for grabs.
    stridx.setTransmit();

    return stridx;
}
