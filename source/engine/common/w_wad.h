// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 4d789c22d9864d8ad84cf5da04532b79b6a6f22e $
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
//	WAD I/O functions.
//
//-----------------------------------------------------------------------------

#pragma once

#include "g_level.h"
#include "m_resfile.h"
#include "physfs.h"
#include "r_defs.h"
#include "z_zone.h"

// [RH] Compare wad header as ints instead of chars
#define IWAD_ID (('I') | ('W' << 8) | ('A' << 16) | ('D' << 24))
#define PWAD_ID (('P') | ('W' << 8) | ('A' << 16) | ('D' << 24))

// [RH] Remove limit on number of WAD files
extern OResFiles  wadfiles;
extern OWantFiles missingfiles;

//
// TYPES
//
typedef struct
{
    // Should be "IWAD" or "PWAD".
    uint32_t identification;
    int32_t  numlumps;
    int32_t  infotableofs;

} wadinfo_t;

typedef struct
{
    int32_t filepos;
    int32_t size;
    char    name[8]; // denis - todo - string

} filelump_t;

//
// WADFILE I/O related stuff.
//
typedef struct lumpinfo_s
{
    char         name[8]; // denis - todo - string
    PHYSFS_File *handle;
    int32_t      position;
    int32_t      size;

    // [RH] Hashing stuff
    int32_t next;
    int32_t index;
} lumpinfo_t;

struct lumpHandle_t
{
    size_t id;
    lumpHandle_t() : id(0)
    {
    }
    void clear()
    {
        id = 0;
    }
    bool empty() const
    {
        return id == 0;
    }
    bool operator==(const lumpHandle_t &other)
    {
        return id == other.id;
    }
};

extern void      **lumpcache;
extern lumpinfo_t *lumpinfo;
extern size_t      numlumps;

OMD5Hash     W_MD5(const std::string &filename);
void         W_InitMultipleFiles(const OResFiles &filenames);
lumpHandle_t W_LumpToHandle(const uint32_t lump);
int32_t      W_HandleToLump(const lumpHandle_t handle);

int32_t W_CheckNumForName(const char *name);
int32_t W_GetNumForName(const char *name);

std::string W_LumpName(uint32_t lump);
uint32_t    W_LumpLength(uint32_t lump);
void        W_ReadLump(uint32_t lump, void *dest);
uint32_t    W_ReadChunk(const char *file, uint32_t offs, uint32_t len, void *dest, uint32_t &filelen);

void *W_CacheLumpNum(uint32_t lump, const zoneTag_e tag);
void *W_CacheLumpName(const char *name, const zoneTag_e tag);

void W_Close();

int32_t W_FindLump(const char *name, int32_t lastlump); // [RH]	Find lumps with duplication
bool    W_CheckLumpName(uint32_t    lump,
                        const char *name); // [RH] True if lump's name == name // denis - todo - replace with map<>

// [RH] Copy an 8-char string and uppercase it.
void uppercopy(char *to, const char *from);

// [RH] Copies the lump name to to using uppercopy
void W_GetLumpName(char *to, uint32_t lump);