//----------------------------------------------------------------------------
//  EDGE EPK Support Code
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

#pragma once

// Open a file from the search order if present, otherwise return nullptr.
// Can either be an explicit path, in which case it will try to be opened
// directly, or a bare filename. open_dirs should be a comma-separated list
// of directory names and, if it is populated, only those directories
// will be searched for a matching bare filename. If it is empty, the bare filename
// will be opened no matter where it resides.
epi::File *OpenPackFile(const std::string &name, std::string_view open_dirs);

// Same as above, but only checks for the existence of a file, does not attempt to open
// or derive any information about it
bool CheckPackFile(const std::string &name, std::string_view check_dirs);

void ProcessPackContents();

// Check /sprites directory for sprites to automatically add during
// InitializeSprites
std::vector<std::string> GetPackSpriteList();

// Populate pack directory and process appropriate files (Lua, DDF, etc)
void ProcessAllInPack(const std::string &df);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
