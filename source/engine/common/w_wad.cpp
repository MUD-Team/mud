// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: e2da070226f8c102eaf42f9c408f70abcfdad859 $
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
//	Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------

#ifdef UNIX
#include <ctype.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#define strcmpi strcasecmp
#endif

#include <algorithm>

#include "Poco/Buffer.h"
#include "cmdlib.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "md5.h"
#include "mud_includes.h"
#include "physfs.h"
#include "r_data.h"
#include "w_wad.h"
#include "z_zone.h"

//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t *lumpinfo;
size_t      numlumps;

// Generation of handle.
// Takes up the first three bits of the handle id.  Starts at 1, increments
// every time we unload the current set of WAD files, and eventually wraps
// around from 7 to 1.  We skip 0 so a handle id of 0 can be considered NULL
// and part of no generation.
size_t       handleGen       = 1;
const size_t HANDLE_GEN_MASK = BIT_MASK(0, 2);
const size_t HANDLE_GEN_BITS = 3;

void **lumpcache;

//
// W_LumpNameHash
//
// Hash function used for lump names. Must be mod'ed with table size.
// Can be used for any 8-character names.
// by Lee Killough
//
// [SL] taken from prboom-plus
//
uint32_t W_LumpNameHash(const char *s)
{
    uint32_t hash;

    (void)((hash = toupper(s[0]), s[1]) && (hash = hash * 3 + toupper(s[1]), s[2]) &&
           (hash = hash * 2 + toupper(s[2]), s[3]) && (hash = hash * 2 + toupper(s[3]), s[4]) &&
           (hash = hash * 2 + toupper(s[4]), s[5]) && (hash = hash * 2 + toupper(s[5]), s[6]) &&
           (hash = hash * 2 + toupper(s[6]), hash = hash * 2 + toupper(s[7])));
    return hash;
}

//
// W_HashLumps
//
// killough 1/31/98: Initialize lump hash table
// [SL] taken from prboom-plus
//
void W_HashLumps(void)
{
    for (uint32_t i = 0; i < numlumps; i++)
        lumpinfo[i].index = -1; // mark slots empty

    // Insert nodes to the beginning of each chain, in first-to-last
    // lump order, so that the last lump of a given name appears first
    // in any chain, observing pwad ordering rules. killough

    for (uint32_t i = 0; i < numlumps; i++)
    {
        uint32_t j    = W_LumpNameHash(lumpinfo[i].name) % (uint32_t)numlumps;
        lumpinfo[i].next  = lumpinfo[j].index; // Prepend to list
        lumpinfo[j].index = i;
    }
}

//
// uppercoppy
//
// [RH] Copy up to 8 chars, upper-casing them in the process
//
void uppercopy(char *to, const char *from)
{
    int32_t i;

    for (i = 0; i < 8 && from[i]; i++)
        to[i] = toupper(from[i]);
    for (; i < 8; i++)
        to[i] = 0;
}

// denis - Standard MD5SUM
OMD5Hash W_MD5(const std::string &filename)
{
    OMD5Hash rvo;

    PHYSFS_File     *fp       = PHYSFS_openRead(filename.c_str());

    if (!fp)
        return rvo;

    size_t len = PHYSFS_fileLength(fp);
    Poco::Buffer<uint8_t> buf(len);

    if (PHYSFS_readBytes(fp, buf.begin(), len) != len)
    {
        PHYSFS_close(fp);
        return rvo;
    }
    else
        PHYSFS_close(fp);

    OMD5Hash::makeFromHexStr(rvo, MD5SUM(buf.begin(), len).c_str());
    return rvo; // bubble up failure
}

//
// LUMP BASED ROUTINES.
//

//
// W_AddLumps
//
// Adds lumps from the array of filelump_t. If clientonly is true,
// only certain lumps will be added.
//
void W_AddLumps(PHYSFS_File *handle, filelump_t *fileinfo, size_t newlumps, bool clientonly)
{
    lumpinfo = (lumpinfo_t *)Realloc(lumpinfo, (numlumps + newlumps) * sizeof(lumpinfo_t));
    if (!lumpinfo)
        I_Error("Couldn't realloc lumpinfo");

    lumpinfo_t *lump = &lumpinfo[numlumps];
    filelump_t *info = &fileinfo[0];

    for (size_t i = 0; i < newlumps; i++, info++)
    {
        lump->handle   = handle;
        lump->position = info->filepos;
        lump->size     = info->size;
        strncpy(lump->name, info->name, 8);

        lump++;
        numlumps++;
    }
}

//
// W_AddFile
//
// All files are optional, but at least one file must be found
// (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files with multiple lumps.
// Other files are single lumps with the base filename for the lump name.
//
// Map reloads are supported through WAD reload so no need for vanilla tilde
// reload hack here
//
void AddFile(const OResFile &file)
{
    if (file.getType() != OFILE_LOOSE)
    {
        if (PHYSFS_mount(file.getFullpath().c_str(), NULL, 0) == 0)
        {
            Printf(PRINT_WARNING, "couldn't mount %s\n", file.getFullpath().c_str());
        }
        return;
    }

    const std::string filename = file.getBasename();
    PHYSFS_File       *handle = NULL;
    filelump_t *fileinfo = NULL;

    if ((handle = PHYSFS_openRead(filename.c_str())) == NULL)
    {
        Printf(PRINT_WARNING, "couldn't open %s\n", filename.c_str());
        return;
    }

    Printf(PRINT_HIGH, "adding %s", filename.c_str());

    size_t newlumps;

    wadinfo_t header;
    size_t    readlen = PHYSFS_readBytes(handle, &header, sizeof(header));
    if (readlen < sizeof(header))
    {
        Printf(PRINT_HIGH, "failed to read %s.\n", filename.c_str());
        PHYSFS_close(handle);
        return;
    }
    header.identification = LELONG(header.identification);

    if (header.identification != IWAD_ID && header.identification != PWAD_ID)
    {
        // raw lump file
        std::string lumpname;
        M_ExtractFileBase(filename, lumpname);

        fileinfo          = new filelump_t[1];
        fileinfo->filepos = 0;
        fileinfo->size    = PHYSFS_fileLength(handle);
        std::transform(lumpname.c_str(), lumpname.c_str() + 8, fileinfo->name, toupper);

        newlumps = 1;
        Printf(PRINT_HIGH, " (single lump)\n");
    }
    else
    {
        // WAD file
        header.numlumps     = LELONG(header.numlumps);
        header.infotableofs = LELONG(header.infotableofs);
        size_t length       = header.numlumps * sizeof(filelump_t);

        if (length > (uint32_t)PHYSFS_fileLength(handle))
        {
            Printf(PRINT_WARNING, "\nbad number of lumps for %s\n", filename.c_str());
            PHYSFS_close(handle);
            return;
        }

        fileinfo = new filelump_t[header.numlumps];
        PHYSFS_seek(handle, header.infotableofs);
        readlen = PHYSFS_readBytes(handle, fileinfo, length);
        if (readlen < length)
        {
            Printf(PRINT_HIGH, "failed to read file info in %s\n", filename.c_str());
            PHYSFS_close(handle);
            return;
        }

        // convert from little-endian to target arch and capitalize lump name
        for (int32_t i = 0; i < header.numlumps; i++)
        {
            fileinfo[i].filepos = LELONG(fileinfo[i].filepos);
            fileinfo[i].size    = LELONG(fileinfo[i].size);
            std::transform(fileinfo[i].name, fileinfo[i].name + 8, fileinfo[i].name, toupper);
        }

        newlumps = header.numlumps;
        Printf(PRINT_HIGH, " (%d lumps)\n", header.numlumps);
    }

    W_AddLumps(handle, fileinfo, newlumps, false);

    delete[] fileinfo;

    return;
}

//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
void W_InitMultipleFiles(const OResFiles &files)
{
    // open all the files, load headers, and count lumps
    // will be realloced as lumps are added
    ::numlumps = 0;

    M_Free(::lumpinfo);
    ::lumpinfo = NULL;

    // open each file once, load headers, and count lumps
    std::vector<OMD5Hash> loaded;
    for (size_t i = 0; i < files.size(); i++)
    {
        if (std::find(loaded.begin(), loaded.end(), files.at(i).getMD5()) == loaded.end())
        {
            AddFile(files.at(i));
            loaded.push_back(files.at(i).getMD5());
        }
    }

    if (!numlumps)
        I_Error("W_InitFiles: no files found");

    // set up caching
    M_Free(lumpcache);

    size_t size = numlumps * sizeof(*lumpcache);
    lumpcache   = (void **)Malloc(size);

    if (!lumpcache)
        I_Error("Couldn't allocate lumpcache");

    memset(lumpcache, 0, size);

    // killough 1/31/98: initialize lump hash table
    W_HashLumps();
}

/**
 * @brief Return a handle for a given lump.
 */
lumpHandle_t W_LumpToHandle(const uint32_t lump)
{
    lumpHandle_t rvo;
    size_t       id = static_cast<size_t>(lump) << HANDLE_GEN_BITS;
    rvo.id          = id | ::handleGen;
    return rvo;
}

/**
 * @brief Return a lump for a given handle, or -1 if the handle is invalid.
 */
int32_t W_HandleToLump(const lumpHandle_t handle)
{
    size_t gen = handle.id & HANDLE_GEN_MASK;
    if (gen != ::handleGen)
    {
        return -1;
    }
    const uint32_t lump = handle.id >> HANDLE_GEN_BITS;
    if (lump >= ::numlumps)
    {
        return -1;
    }
    return lump;
}

//
// W_CheckNumForName
// Returns -1 if name not found.
//
// Rewritten by Lee Killough to use hash table for performance. Significantly
// cuts down on time -- increases Doom performance over 300%. This is the
// single most important optimization of the original Doom sources, because
// lump name lookup is used so often, and the original Doom used a sequential
// search. For large wads with > 1000 lumps this meant an average of over
// 500 were probed during every search. Now the average is under 2 probes per
// search. There is no significant benefit to packing the names into longwords
// with this new hashing algorithm, because the work to do the packing is
// just as much work as simply doing the string comparisons with the new
// algorithm, which minimizes the expected number of comparisons to under 2.
//
// [SL] taken from prboom-plus
//
int32_t W_CheckNumForName(const char *name)
{
    // Hash function maps the name to one of possibly numlump chains.
    // It has been tuned so that the average chain length never exceeds 2.

    // proff 2001/09/07 - check numlumps==0, this happens when called before WAD loaded
    int32_t i = (numlumps == 0) ? (-1) : (lumpinfo[W_LumpNameHash(name) % numlumps].index);

    while (i >= 0 && strnicmp(lumpinfo[i].name, name, 8))
        i = lumpinfo[i].next;

    // Return the matching lump, or -1 if none found.
    return i;
}

//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int32_t W_GetNumForName(const char *name)
{
    int32_t i = W_CheckNumForName(name);

    if (i == -1)
    {
        I_Error("W_GetNumForName: %s not found!\n(checked in: %s)", name, M_ResFilesToString(::wadfiles).c_str());
    }

    return i;
}

/**
 * @brief Return the name of a lump number.
 *
 * @detail You likely only need this for debugging, since a name can be
 *         ambiguous.
 */
std::string W_LumpName(uint32_t lump)
{
    if (lump >= ::numlumps)
        I_Error("%s: %i >= numlumps", __FUNCTION__, lump);

    return std::string(::lumpinfo[lump].name, ARRAY_LENGTH(::lumpinfo[lump].name));
}

//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
uint32_t W_LumpLength(uint32_t lump)
{
    if (lump >= numlumps)
        I_Error("W_LumpLength: %i >= numlumps", lump);

    return lumpinfo[lump].size;
}

//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void W_ReadLump(uint32_t lump, void *dest)
{
    int32_t         c;
    lumpinfo_t *l;

    if (lump >= numlumps)
        I_Error("W_ReadLump: %i >= numlumps", lump);

    l = lumpinfo + lump;

    I_BeginRead();

    PHYSFS_seek(l->handle, l->position);
    c = PHYSFS_readBytes(l->handle, dest, l->size);

    if (PHYSFS_eof(l->handle))
        I_Error("W_ReadLump: only read %i of %i on lump %i", c, l->size, lump);

    I_EndRead();
}

//
// W_ReadChunk
//
// denis - for wad downloading
//
uint32_t W_ReadChunk(const char *file, uint32_t offs, uint32_t len, void *dest, uint32_t &filelen)
{
    PHYSFS_File    *fp   = PHYSFS_openRead(file);
    uint32_t read = 0;

    if (fp)
    {
        filelen = PHYSFS_fileLength(fp);

        PHYSFS_seek(fp, offs);
        read = PHYSFS_readBytes(fp, dest, len);
        PHYSFS_close(fp);
    }
    else
        filelen = 0;

    return read;
}

//
// W_CheckLumpName
//
bool W_CheckLumpName(uint32_t lump, const char *name)
{
    if (lump >= numlumps)
        return false;

    return !strnicmp(lumpinfo[lump].name, name, 8);
}

//
// W_GetLumpName
//
void W_GetLumpName(char *to, uint32_t lump)
{
    if (lump >= numlumps)
        *to = 0;
    else
    {
        memcpy(to, lumpinfo[lump].name, 8); // denis - todo -string limit?
        to[8] = '\0';
        std::transform(to, to + strlen(to), to, toupper);
    }
}

//
// W_CacheLumpNum
//
void *W_CacheLumpNum(uint32_t lump, const zoneTag_e tag)
{
    if ((uint32_t)lump >= numlumps)
        I_Error("W_CacheLumpNum: %i >= numlumps", lump);

    if (!lumpcache[lump])
    {
        // read the lump in
        // [RH] Allocate one byte more than necessary for the
        //		lump and set the extra byte to zero so that
        //		various text parsing routines can just call
        //		W_CacheLumpNum() and not choke.

        // DPrintf("cache miss on lump %i\n",lump);
        uint32_t lump_length = W_LumpLength(lump);
        lumpcache[lump]          = (uint8_t *)Z_Malloc(lump_length + 1, tag, &lumpcache[lump]);
        W_ReadLump(lump, lumpcache[lump]);
        *((uint8_t *)lumpcache[lump] + lump_length) = 0;
    }
    else
    {
        // printf ("cache hit on lump %i\n",lump);
        Z_ChangeTag(lumpcache[lump], tag);
    }

    return lumpcache[lump];
}

//
// W_CacheLumpName
//
void *W_CacheLumpName(const char *name, const zoneTag_e tag)
{
    return W_CacheLumpNum(W_GetNumForName(name), tag);
}

//
// W_FindLump
//
// Find a named lump.
//
// [SL] 2013-01-15 - Search forwards through the list of lumps in reverse pwad
// ordering, returning older lumps with a matching name first.
// Initialize search with lastlump == -1 before calling for the first time.
//
int32_t W_FindLump(const char *name, int32_t lastlump)
{
    if (lastlump < -1)
        lastlump = -1;

    for (int32_t i = lastlump + 1; i < (int32_t)numlumps; i++)
    {
        if (strnicmp(lumpinfo[i].name, name, 8) == 0)
            return i;
    }

    return -1;
}

//
// W_Close
//
// Close all open files
//

void W_Close()
{
    // store closed handles, so that fclose isn't called multiple times
    // for the same handle
    std::vector<PHYSFS_File *> handles;

    lumpinfo_t *lump_p = lumpinfo;
    while (lump_p < lumpinfo + numlumps)
    {
        // if file not previously closed, close it now
        if (lump_p->handle && std::find(handles.begin(), handles.end(), lump_p->handle) == handles.end())
        {
            PHYSFS_close(lump_p->handle);
            handles.push_back(lump_p->handle);
        }
        lump_p++;
    }

    ::handleGen = (::handleGen + 1) & HANDLE_GEN_MASK;
    if (::handleGen == 0)
    {
        // 0 is reserved for the NULL handle.
        ::handleGen += 1;
    }
}

VERSION_CONTROL(w_wad_cpp, "$Id: e2da070226f8c102eaf42f9c408f70abcfdad859 $")
