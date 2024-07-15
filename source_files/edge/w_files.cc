//----------------------------------------------------------------------------
//  EDGE file handling
//----------------------------------------------------------------------------
//
//  Copyright (c) 2022-2024 The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "w_files.h"

#include <algorithm>
#include <list>
#include <vector>

#include "con_main.h"
#include "ddf_anim.h"
#include "ddf_colormap.h"
#include "ddf_flat.h"
#include "ddf_font.h"
#include "ddf_image.h"
#include "ddf_main.h"
#include "ddf_style.h"
#include "ddf_switch.h"
#include "dm_state.h"
#include "dstrings.h"
#include "epi.h"
#include "epi_filesystem.h"
#include "i_system.h"
#include "w_epk.h"


std::vector<std::string> data_files;

int GetTotalFiles()
{
    return (int)data_files.size();
}

size_t AddDataFile(const std::string &file)
{
    LogDebug("Added filename: %s\n", file.c_str());

    size_t index = data_files.size();

    data_files.push_back(file);

    return index;
}

//----------------------------------------------------------------------------

void ProcessFile(const std::string &df)
{
    LogPrint("  Processing: %s\n", df.c_str());

    ProcessAllInPack(df);
}

void ProcessMultipleFiles()
{
    for (size_t i = 0; i < data_files.size(); i++)
        ProcessFile(data_files[i]);
}

//----------------------------------------------------------------------------

void ShowLoadedFiles()
{
    LogPrint("File list:\n");

    for (int i = 0; i < (int)data_files.size(); i++)
    {
        LogPrint(" %2d:  \"%s\"\n", i + 1, epi::SanitizePath(data_files[i]).c_str());
    }
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
