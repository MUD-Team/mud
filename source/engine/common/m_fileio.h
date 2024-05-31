// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ec30f73ffd415d2b1df1bb40554f2643495612da $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	File Input/Output Operations
//
//-----------------------------------------------------------------------------

#pragma once

#include <fstream>

#include "d_main.h"
#include "m_ostring.h"
#include "physfs.h"

extern std::ofstream LOG;
extern std::ifstream CON;

std::string M_FindUserFileName(const std::string &file, const char *ext);
void        M_FixPathSep(std::string &path);

bool   M_FileExists(const std::string &filename);
bool   M_FileExistsExt(const std::string &filename, const char *ext);

bool        M_AppendExtension(std::string &filename, std::string extension, bool if_needed = true);
void        M_ExtractFilePath(const std::string &filename, std::string &dest);
bool        M_ExtractFileExtension(const std::string &filename, std::string &dest);
void        M_ExtractFileBase(std::string filename, std::string &dest);
void        M_ExtractFileName(std::string filename, std::string &dest);
std::string M_ExtractFileName(const std::string &filename);
bool        M_IsPathSep(const char ch);
std::string M_CleanPath(std::string path);

/*
 * OS-specific functions are below.
 */

/**
 * @brief Get the directory of the Odamex binary.
 */
std::string M_GetBinaryDir();

/**
 * @brief Get the directory that files such as game config and screenshots
 *        shall be written into.  If the directory does not exist, it will
 *        be created.  This function also accounts for portable installations.
 */
std::string M_GetWriteDir();

/**
 * @brief Resolve a file name into a user directory.
 *
 * @detail This function is OS-specific.
 *
 *         Aboslute and relative paths that begin with "." and ".." should
 *         be resolved relative to the Odamex binary.  Otherwise, the file
 *         is resolved relative to a platform and user specific directory
 *         in their home directory.
 *
 *         The resolution process will create a user home directory if it
 *         doesn't exist, but otherwise this process is blind and does not
 *         consider the existence of the file in question.
 *
 * @param file Filename to resolve.
 * @return An absolute path pointing to the resolved file.
 */
std::string M_GetUserFileName(const std::string &file);