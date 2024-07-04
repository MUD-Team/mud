//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Main)
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2024 The EDGE Team.
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
// See the file "docs/save_sys.txt" for a complete description of the
// new savegame system.
//

#include "sv_main.h"

#include "dm_state.h"
#include "dstrings.h"
#include "e_main.h"
#include "epi.h"
#include "epi_filesystem.h"
#include "epi_str_util.h"
#include "f_interm.h"
#include "g_game.h"
#include "m_math.h"
#include "m_random.h"
#include "p_local.h"
#include "p_spec.h"
#include "r_state.h"
#include "w_wad.h"

const char *SaveSlotName(int slot)
{
    EPI_ASSERT(slot < 1000);

    static char buffer[256];

    sprintf(buffer, "slot%03d", slot);

    return buffer;
}

const char *SaveMapName(const MapDefinition *map)
{
    // ensure the name is LOWER CASE
    static char buffer[256];

    strcpy(buffer, map->name_.c_str());

    for (char *pos = buffer; *pos; pos++)
        *pos = epi::ToLowerASCII(*pos);

    return buffer;
}

std::string SaveFilename(const char *slot_name, const char *map_name)
{
    std::string temp(epi::StringFormat("%s/%s.%s", slot_name, map_name, kSaveGameExtension));

    return epi::PathAppend(save_directory, temp);
}

std::string SV_DirName(const char *slot_name)
{
    return epi::PathAppend(save_directory, slot_name);
}

void SaveClearSlot(const char *slot_name)
{
// Stubbed out pending save system rework - Dasho
    return;
/*
    std::string full_dir = SV_DirName(slot_name);

    // make sure the directory exists
    epi::MakeDirectory(full_dir);

    std::vector<epi::DirectoryEntry> fsd;

    if (!ReadDirectory(fsd, full_dir, "*.*"))
    {
        LogDebug("Failed to read directory: %s\n", full_dir.c_str());
        return;
    }

    LogDebug("SV_ClearSlot: removing %d files\n", (int)fsd.size());

    for (size_t i = 0; i < fsd.size(); i++)
    {
        if (fsd[i].is_dir)
            continue;
        std::string cur_file = epi::PathAppend(full_dir, epi::GetFilename(fsd[i].name));
        LogDebug("  Deleting %s\n", cur_file.c_str());

        epi::FileDelete(cur_file);
    }*/
}

void SaveCopySlot(const char *src_name, const char *dest_name)
{
// Stubbed out pending save system rework - Dasho
    return;

    /*std::string src_dir  = SV_DirName(src_name);
    std::string dest_dir = SV_DirName(dest_name);

    std::vector<epi::DirectoryEntry> fsd;

    if (!ReadDirectory(fsd, src_dir, "*.*"))
    {
        FatalError("SV_CopySlot: failed to read dir: %s\n", src_dir.c_str());
        return;
    }

    LogDebug("SV_CopySlot: copying %d files\n", (int)fsd.size());

    for (size_t i = 0; i < fsd.size(); i++)
    {
        if (fsd[i].is_dir)
            continue;

        std::string fn        = epi::GetFilename(fsd[i].name);
        std::string src_file  = epi::PathAppend(src_dir, fn);
        std::string dest_file = epi::PathAppend(dest_dir, fn);

        LogDebug("  Copying %s --> %s\n", src_file.c_str(), dest_file.c_str());

        if (!epi::FileCopy(src_file, dest_file))
            FatalError("SV_CopySlot: failed to copy '%s' to '%s'\n", src_file.c_str(), dest_file.c_str());
    }*/
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
