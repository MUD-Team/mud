//----------------------------------------------------------------------------
//  EDGE WAD Support Code
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// This file contains various levels of support for using sprites and
// flats directly from a PWAD as well as some minor optimisations for
// patches. Because there are some PWADs that do arcane things with
// sprites, it is possible that this feature may not always work (at
// least, not until I become aware of them and support them) and so
// this feature can be turned off from the command line if necessary.
//
// -MH- 1998/03/04
//

#include "w_wad.h"

#include <limits.h>

#include <algorithm>
#include <list>
#include <vector>

#include "bsp.h"
#include "ddf_anim.h"
#include "ddf_colormap.h"
#include "ddf_main.h"
#include "ddf_switch.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "e_main.h"
#include "e_search.h"
#include "epi_doomdefs.h"
#include "epi_endian.h"
#include "epi_file.h"
#include "epi_filesystem.h"
#include "epi_md5.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "i_system.h"
#include "m_misc.h"
#include "r_image.h"
#include "script/compat/lua_compat.h"
#include "w_epk.h"
#include "w_files.h"

class WadFile
{
  public:
    // lists for sprites, flats, patches (stuff between markers)
    std::vector<int> colormap_lumps_;
    std::vector<int> xgl_lumps_;

    // level markers and skin markers
    std::vector<int> level_markers_;
    std::vector<int> skin_markers_;

    // ddf and rts lump list
    int ddf_lumps_[kTotalDDFTypes];

    // LUA scripts
    int lua_huds_;

    // BOOM stuff
    int animated_;
    int switches_;

    std::string md5_string_;

  public:
    WadFile()
        : colormap_lumps_(), xgl_lumps_(),
          level_markers_(), skin_markers_(), lua_huds_(-1),
          animated_(-1), switches_(-1), md5_string_()
    {
        for (int d = 0; d < kTotalDDFTypes; d++)
            ddf_lumps_[d] = -1;
    }

    ~WadFile()
    {
    }

    bool HasLevel(const char *name) const;
};

enum LumpKind
{
    kLumpNormal   = 0,  // fallback value
    kLumpMarker   = 3,  // X_START, X_END, S_SKIN, level name
    kLumpDDF      = 10, // DDF, RTS, Lua lump
    kLumpColormap = 15,
    kLumpXGL      = 20,
};

struct LumpInfo
{
    char name[10];

    int position;
    int size;

    // file number (an index into data_files[]).
    int file;

    // one of the LMKIND values.  For sorting, this is the least
    // significant aspect (but still necessary).
    LumpKind kind;
};

//
//  GLOBALS
//

// Location of each lump on disk.
static std::vector<LumpInfo> lump_info;

static std::vector<int> sorted_lumps;

// the first datafile which contains a PLAYPAL lump
static int palette_datafile = -1;

static bool within_colmap_list;
static bool within_xgl_list;

//
// Is the name a colourmap list start/end flag?
//
static bool IsC_START(char *name)
{
    return (strncmp(name, "C_START", 8) == 0);
}

static bool IsC_END(char *name)
{
    return (strncmp(name, "C_END", 8) == 0);
}

//
// Is the name a XGL nodes start/end flag?
//
static bool IsXG_START(char *name)
{
    return (strncmp(name, "XG_START", 8) == 0);
}

static bool IsXG_END(char *name)
{
    return (strncmp(name, "XG_END", 8) == 0);
}

//
// Is the name a skin specifier ?
//
static bool IsSkin(const char *name)
{
    return (strncmp(name, "S_SKIN", 6) == 0);
}

bool WadFile::HasLevel(const char *name) const
{
    for (size_t i = 0; i < level_markers_.size(); i++)
        if (strcmp(lump_info[level_markers_[i]].name, name) == 0)
            return true;

    return false;
}

//
// SortLumps
//
// Create the sorted_lumps array, which is sorted by name for fast
// searching.  When two names are the same, we prefer lumps in later
// WADs over those in earlier ones.
//
// -AJA- 2000/10/14: simplified.
//
struct Compare_lump_pred
{
    inline bool operator()(const int &A, const int &B) const
    {
        const LumpInfo &C = lump_info[A];
        const LumpInfo &D = lump_info[B];

        // increasing name
        int cmp = strcmp(C.name, D.name);
        if (cmp != 0)
            return (cmp < 0);

        // decreasing file number
        cmp = C.file - D.file;
        if (cmp != 0)
            return (cmp > 0);

        // lump type
        if (C.kind != D.kind)
            return C.kind > D.kind;

        // tie breaker
        return C.position > D.position;
    }
};

static void SortLumps(void)
{
    int i;

    sorted_lumps.resize(lump_info.size());

    for (i = 0; i < (int)lump_info.size(); i++)
        sorted_lumps[i] = i;

    // sort it, primarily by increasing name, secondly by decreasing
    // file number, thirdly by the lump type.

    std::sort(sorted_lumps.begin(), sorted_lumps.end(), Compare_lump_pred());
}

//
// LUMP BASED ROUTINES.
//

//
// AddLump
//
static void AddLump(DataFile *df, const char *raw_name, int pos, int size, int file_index, bool allow_ddf)
{
    int lump = (int)lump_info.size();

    LumpInfo info;

    info.position = pos;
    info.size     = size;
    info.file     = file_index;
    info.kind     = kLumpNormal;

    // copy name, make it uppercase
    strncpy(info.name, raw_name, 8);
    info.name[8] = 0;

    for (size_t i = 0; i < strlen(info.name); i++)
    {
        info.name[i] = epi::ToUpperASCII(info.name[i]);
    }

    lump_info.push_back(info);

    LumpInfo *lump_p = &lump_info.back();

    // -- handle special names --

    WadFile *wad = df->wad_;

    if (strcmp(info.name, "PLAYPAL") == 0)
    {
        if (palette_datafile < 0)
            palette_datafile = file_index;
        return;
    }
    else if (strcmp(info.name, "LUAHUDS") == 0)
    {
        lump_p->kind = kLumpDDF;
        if (wad != nullptr)
            wad->lua_huds_ = lump;
        return;
    }
    else if (strcmp(info.name, "ANIMATED") == 0)
    {
        lump_p->kind = kLumpDDF;
        if (wad != nullptr)
            wad->animated_ = lump;
        return;
    }
    else if (strcmp(info.name, "SWITCHES") == 0)
    {
        lump_p->kind = kLumpDDF;
        if (wad != nullptr)
            wad->switches_ = lump;
        return;
    }

    // -KM- 1998/12/16 Load DDF/RSCRIPT file from wad.
    if (allow_ddf && wad != nullptr)
    {
        DDFType type = DDFLumpToType(info.name);

        if (type != kDDFTypeUnknown)
        {
            lump_p->kind          = kLumpDDF;
            wad->ddf_lumps_[type] = lump;
            return;
        }
    }

    if (IsSkin(info.name))
    {
        lump_p->kind = kLumpMarker;
        if (wad != nullptr)
            wad->skin_markers_.push_back(lump);
        return;
    }

    // -- handle sprite, flat & patch lists --

    if (IsC_START(lump_p->name))
    {
        lump_p->kind       = kLumpMarker;
        within_colmap_list = true;
        return;
    }
    else if (IsC_END(lump_p->name))
    {
        if (!within_colmap_list)
            LogWarning("Unexpected C_END marker in wad.\n");

        lump_p->kind       = kLumpMarker;
        within_colmap_list = false;
        return;
    }
    else if (IsXG_START(lump_p->name))
    {
        lump_p->kind    = kLumpMarker;
        within_xgl_list = true;
        return;
    }
    else if (IsXG_END(lump_p->name))
    {
        if (!within_xgl_list)
            LogWarning("Unexpected XG_END marker in wad.\n");

        lump_p->kind    = kLumpMarker;
        within_xgl_list = false;
        return;
    }

    // ignore zero size lumps or dummy markers
    if (lump_p->size == 0)
        return;

    if (wad == nullptr)
        return;

    if (within_colmap_list)
    {
        lump_p->kind = kLumpColormap;
        wad->colormap_lumps_.push_back(lump);
    }

    if (within_xgl_list)
    {
        lump_p->kind = kLumpXGL;
        wad->xgl_lumps_.push_back(lump);
    }
}

//
// CheckForLevel
//
// Tests whether the current lump is a level marker (MAP03, E1M7, etc).
// Because EDGE supports arbitrary names (via DDF), we look at the
// sequence of lumps _after_ this one, which works well since their
// order is fixed (e.g. THINGS is always first).
//
static void CheckForLevel(WadFile *wad, int lump, const char *name, const RawWadEntry *raw, int remaining)
{
    // we only test four lumps (it is enough), but fewer definitely
    // means this is not a level marker.
    if (remaining < 2)
        return;

    if (strncmp(raw[1].name, "THINGS", 8) == 0 && strncmp(raw[2].name, "LINEDEFS", 8) == 0 &&
        strncmp(raw[3].name, "SIDEDEFS", 8) == 0 && strncmp(raw[4].name, "VERTEXES", 8) == 0)
    {
        if (strlen(name) > 5)
        {
            LogWarning("Level name '%s' is too long !!\n", name);
            return;
        }

        // check for duplicates (Slige sometimes does this)
        if (wad->HasLevel(name))
        {
            LogWarning("Duplicate level '%s' ignored.\n", name);
            return;
        }

        wad->level_markers_.push_back(lump);
        return;
    }

    // handle GL nodes here too

    if (strncmp(raw[1].name, "GL_VERT", 8) == 0 && strncmp(raw[2].name, "GL_SEGS", 8) == 0 &&
        strncmp(raw[3].name, "GL_SSECT", 8) == 0 && strncmp(raw[4].name, "GL_NODES", 8) == 0)
    {
        wad->level_markers_.push_back(lump);
        return;
    }

    // UDMF
    // 1.1 Doom/Heretic namespaces supported at the moment

    if (strncmp(raw[1].name, "TEXTMAP", 8) == 0)
    {
        wad->level_markers_.push_back(lump);
        return;
    }
}

std::string CheckForEdgeGameLump(epi::File *file)
{
    int          length;
    RawWadHeader header;
    std::string  game_name;

    if (!file)
    {
        LogWarning("CheckForEdgeGameLump: Received null file_c pointer!\n");
        return game_name;
    }

    // WAD file
    // TODO: handle Read failure
    file->Read(&header, sizeof(RawWadHeader));
    header.total_entries   = AlignedLittleEndianS32(header.total_entries);
    header.directory_start = AlignedLittleEndianS32(header.directory_start);
    length                 = header.total_entries * sizeof(RawWadEntry);
    RawWadEntry *raw_info  = new RawWadEntry[header.total_entries];
    file->Seek(header.directory_start, epi::File::kSeekpointStart);
    file->Read(raw_info, length);

    for (size_t i = 0; i < header.total_entries; i++)
    {
        RawWadEntry &entry = raw_info[i];

        if (epi::StringCompare("EDGEGAME", entry.name) == 0)
        {
            std::string edge_game;
            edge_game.resize(entry.size);
            file->Seek(entry.position, epi::File::kSeekpointStart);
            file->Read(game_name.data(), entry.size);
            epi::Lexer lex(edge_game);
            game_name = ParseEdgeGameFile(lex);
            delete[] raw_info;
            file->Seek(0, epi::File::kSeekpointStart);
            return game_name;
        }
    }

    delete[] raw_info;
    file->Seek(0, epi::File::kSeekpointStart);
    return game_name;
}

static void ProcessDDFInWad(DataFile *df)
{
    std::string bare_filename = epi::GetFilename(df->name_);

    for (size_t d = 0; d < kTotalDDFTypes; d++)
    {
        int lump = df->wad_->ddf_lumps_[d];

        if (lump >= 0)
        {
            LogPrint("Loading %s lump in %s\n", GetLumpNameFromIndex(lump), bare_filename.c_str());

            std::string data   = LoadLumpAsString(lump);
            std::string source = GetLumpNameFromIndex(lump);

            source += " in ";
            source += bare_filename;

            DDFAddFile((DDFType)d, data, source);
        }
    }
}

static void ProcessLuaInWad(DataFile *df)
{
    std::string bare_filename = epi::GetFilename(df->name_);

    WadFile *wad = df->wad_;

    if (wad->lua_huds_ >= 0)
    {
        int lump = wad->lua_huds_;

        std::string data   = LoadLumpAsString(lump);
        std::string source = GetLumpNameFromIndex(lump);

        source += " in ";
        source += bare_filename;

        LuaAddScript(data, source);
    }
}

static void ProcessBoomStuffInWad(DataFile *df)
{
    // handle Boom's ANIMATED and SWITCHES lumps

    int animated = df->wad_->animated_;
    int switches = df->wad_->switches_;

    if (animated >= 0)
    {
        LogPrint("Loading ANIMATED from: %s\n", df->name_.c_str());

        int      length = -1;
        uint8_t *data   = LoadLumpIntoMemory(animated, &length);

        DDFConvertAnimatedLump(data, length);
        delete[] data;
    }

    if (switches >= 0)
    {
        LogPrint("Loading SWITCHES from: %s\n", df->name_.c_str());

        int      length = -1;
        uint8_t *data   = LoadLumpIntoMemory(switches, &length);

        DDFConvertSwitchesLump(data, length);
        delete[] data;
    }

    // handle BOOM Colourmaps (between C_START and C_END)
    for (int lump : df->wad_->colormap_lumps_)
    {
        DDFAddRawColourmap(GetLumpNameFromIndex(lump), GetLumpLength(lump), nullptr, lump);
    }
}

void ProcessWad(DataFile *df, size_t file_index)
{
    WadFile *wad = new WadFile();
    df->wad_     = wad;

    // reset the colormap/node list stuff
    within_colmap_list = within_xgl_list = false;

    RawWadHeader header;

    epi::File *file = df->file_;

    // TODO: handle Read failure
    file->Read(&header, sizeof(RawWadHeader));

    if (strncmp(header.magic, "IWAD", 4) != 0)
    {
        // Homebrew levels?
        if (strncmp(header.magic, "PWAD", 4) != 0)
        {
            FatalError("Wad file %s doesn't have IWAD or PWAD id\n", df->name_.c_str());
        }
    }

    header.total_entries   = AlignedLittleEndianS32(header.total_entries);
    header.directory_start = AlignedLittleEndianS32(header.directory_start);

    size_t length = header.total_entries * sizeof(RawWadEntry);

    RawWadEntry *raw_info = new RawWadEntry[header.total_entries];

    file->Seek(header.directory_start, epi::File::kSeekpointStart);
    // TODO: handle Read failure
    file->Read(raw_info, length);

    int startlump = (int)lump_info.size();

    for (size_t i = 0; i < header.total_entries; i++)
    {
        RawWadEntry &entry = raw_info[i];

        bool allow_ddf = true;

        AddLump(df, entry.name, AlignedLittleEndianS32(entry.position), AlignedLittleEndianS32(entry.size),
                (int)file_index, allow_ddf);

        // this will be uppercase
        const char *level_name = lump_info[startlump + i].name;

        CheckForLevel(wad, startlump + i, level_name, &entry, header.total_entries - 1 - i);
    }

    // check for unclosed sprite/flat/patch lists
    const char *filename = df->name_.c_str();
    if (within_colmap_list)
        LogWarning("Missing C_END marker in %s.\n", filename);
    if (within_xgl_list)
        LogWarning("Missing XG_END marker in %s.\n", filename);

    SortLumps();

    // compute MD5 hash over wad directory
    epi::MD5Hash dir_md5;
    dir_md5.Compute((const uint8_t *)raw_info, length);

    wad->md5_string_ = dir_md5.ToString();

    LogDebug("   md5hash = %s\n", wad->md5_string_.c_str());

    delete[] raw_info;

    ProcessBoomStuffInWad(df);
    ProcessDDFInWad(df);
    ProcessLuaInWad(df);
}

std::string BuildXGLNodesForWAD(DataFile *df)
{
    if (df->wad_->level_markers_.empty())
        return "";

    // determine XWA filename in the cache
    std::string cache_name = epi::GetStem(df->name_);
    cache_name += "-";
    cache_name += df->wad_->md5_string_;
    cache_name += ".xwa";

    std::string xwa_filename = epi::PathAppend(cache_directory, cache_name);

    LogDebug("XWA filename: %s\n", xwa_filename.c_str());

    // check whether an XWA file for this map exists in the cache
    bool exists = epi::TestFileAccess(xwa_filename);

    if (!exists)
    {
        LogPrint("Building XGL nodes for: %s\n", df->name_.c_str());

        LogDebug("# source: '%s'\n", df->name_.c_str());
        LogDebug("#   dest: '%s'\n", xwa_filename.c_str());

        ajbsp::ResetInfo();

        epi::File *mem_wad    = nullptr;
        uint8_t   *raw_wad    = nullptr;
        int        raw_length = 0;

        if (df->kind_ == kFileKindPackWAD || df->kind_ == kFileKindIPackWAD)
        {
            mem_wad    = OpenFileFromPack(df->name_);
            raw_length = mem_wad->GetLength();
            raw_wad    = mem_wad->LoadIntoMemory();
            ajbsp::OpenMem(df->name_, raw_wad, raw_length);
        }
        else
            ajbsp::OpenWad(df->name_);

        ajbsp::CreateXWA(xwa_filename);

        for (int i = 0; i < ajbsp::LevelsInWad(); i++)
            ajbsp::BuildLevel(i);

        ajbsp::FinishXWA();
        ajbsp::CloseWad();

        if (df->kind_ == kFileKindPackWAD || df->kind_ == kFileKindIPackWAD)
        {
            delete[] raw_wad;
            delete mem_wad;
        }

        LogDebug("AJ_BuildNodes: FINISHED\n");        
    }

    return xwa_filename;
}

epi::File *LoadLumpAsFile(int lump)
{
    EPI_ASSERT(IsLumpIndexValid(lump));

    LumpInfo *l = &lump_info[lump];

    DataFile *df = data_files[l->file];

    EPI_ASSERT(df->file_);

    return new epi::SubFile(df->file_, l->position, l->size);
}

epi::File *LoadLumpAsFile(const char *name)
{
    return LoadLumpAsFile(GetLumpNumberForName(name));
}

//
// GetPaletteForLump
//
// Returns the palette lump that should be used for the given lump
// (presumably an image), otherwise -1 (indicating that the global
// palette should be used).
//
// NOTE: when the same WAD as the lump does not contain a palette,
// there are two possibilities: search backwards for the "closest"
// palette, or simply return -1.  Neither one is ideal, though I tend
// to think that searching backwards is more intuitive.
//
// NOTE 2: the palette_datafile stuff is there so we always return -1
// for the "GLOBAL" palette.
//
int GetPaletteForLump(int lump)
{
    EPI_ASSERT(IsLumpIndexValid(lump));

    return CheckLumpNumberForName("PLAYPAL");
}

static int QuickFindLumpMap(const char *buf)
{
    int low  = 0;
    int high = (int)lump_info.size() - 1;

    if (high < 0)
        return -1;

    while (low <= high)
    {
        int i   = (low + high) / 2;
        int cmp = epi::StringCompareMax(lump_info[sorted_lumps[i]].name, buf, 8);

        if (cmp == 0)
        {
            // jump to first matching name
            while (i > 0 && epi::StringCompareMax(lump_info[sorted_lumps[i - 1]].name, buf, 8) == 0)
                i--;

            return i;
        }

        if (cmp < 0)
        {
            // mid point < buf, so look in upper half
            low = i + 1;
        }
        else
        {
            // mid point > buf, so look in lower half
            high = i - 1;
        }
    }

    // not found (nothing has that name)
    return -1;
}

//
// CheckLumpNumberForName
//
// Returns -1 if name not found.
//
// -ACB- 1999/09/18 Added name to error message
//
int CheckLumpNumberForName(const char *name)
{
    int  i;
    char buf[9];

    if (strlen(name) > 8)
    {
        LogDebug("CheckLumpNumberForName: Name '%s' longer than 8 chars!\n", name);
        return -1;
    }

    for (i = 0; name[i]; i++)
    {
        buf[i] = epi::ToUpperASCII(name[i]);
    }
    buf[i] = 0;

    i = QuickFindLumpMap(buf);

    if (i < 0)
        return -1; // not found

    return sorted_lumps[i];
}

//
// CheckDataFileIndexForName
//
// Returns data_files index or -1 if name not found.
//
//
int CheckDataFileIndexForName(const char *name)
{
    int  i;
    char buf[9];

    if (strlen(name) > 8)
    {
        LogDebug("CheckLumpNumberForName: Name '%s' longer than 8 chars!\n", name);
        return -1;
    }

    for (i = 0; name[i]; i++)
    {
        buf[i] = epi::ToUpperASCII(name[i]);
    }
    buf[i] = 0;

    i = QuickFindLumpMap(buf);

    if (i < 0)
        return -1; // not found

    return lump_info[sorted_lumps[i]].file;
}

int CheckXGLLumpNumberForName(const char *name)
{
    // limit search to stuff between XG_START and XG_END.

    int  i;
    char buf[9];

    if (strlen(name) > 8)
    {
        LogWarning("CheckLumpNumberForName: Name '%s' longer than 8 chars!\n", name);
        return -1;
    }

    for (i = 0; name[i]; i++)
    {
        buf[i] = epi::ToUpperASCII(name[i]);
    }
    buf[i] = 0;

    // search backwards
    for (i = (int)lump_info.size() - 1; i >= 0; i--)
    {
        if (lump_info[i].kind == kLumpXGL)
            if (strncmp(lump_info[i].name, buf, 8) == 0)
                return i;
    }

    return -1; // not found
}

int CheckMapLumpNumberForName(const char *name)
{
    // avoids anything in XGL namespace

    int  i;
    char buf[9];

    if (strlen(name) > 8)
    {
        LogWarning("CheckLumpNumberForName: Name '%s' longer than 8 chars!\n", name);
        return -1;
    }

    for (i = 0; name[i]; i++)
    {
        buf[i] = epi::ToUpperASCII(name[i]);
    }
    buf[i] = 0;

    // search backwards
    for (i = (int)lump_info.size() - 1; i >= 0; i--)
    {
        if (lump_info[i].kind != kLumpXGL)
            if (strncmp(lump_info[i].name, buf, 8) == 0)
                return i;
    }

    return -1; // not found
}

//
// GetLumpNumberForName
//
// Calls CheckLumpNumberForName, but bombs out if not found.
//
int GetLumpNumberForName(const char *name)
{
    int i;

    if ((i = CheckLumpNumberForName(name)) == -1)
        FatalError("GetLumpNumberForName: \'%.8s\' not found!", name);

    return i;
}

//
// IsLumpIndexValid
//
// Verifies that the given lump number is valid and has the given
// name.
//
// -AJA- 1999/11/26: written.
//
bool IsLumpIndexValid(int lump)
{
    return (lump >= 0) && (lump < (int)lump_info.size());
}

bool VerifyLump(int lump, const char *name)
{
    if (!IsLumpIndexValid(lump))
        return false;

    return (strncmp(lump_info[lump].name, name, 8) == 0);
}

//
// GetLumpLength
//
// Returns the buffer size needed to load the given lump.
//
int GetLumpLength(int lump)
{
    if (!IsLumpIndexValid(lump))
        FatalError("GetLumpLength: %i >= numlumps", lump);

    return lump_info[lump].size;
}

int GetDataFileIndexForLump(int lump)
{
    EPI_ASSERT(IsLumpIndexValid(lump));

    return lump_info[lump].file;
}

int GetKindForLump(int lump)
{
    EPI_ASSERT(IsLumpIndexValid(lump));

    return lump_info[lump].kind;
}

//
// Loads the lump into the given buffer,
// which must be >= GetLumpLength().
//
static void W_RawReadLump(int lump, void *dest)
{
    if (!IsLumpIndexValid(lump))
        FatalError("W_ReadLump: %i >= numlumps", lump);

    LumpInfo *L  = &lump_info[lump];
    DataFile *df = data_files[L->file];

    df->file_->Seek(L->position, epi::File::kSeekpointStart);

    int c = df->file_->Read(dest, L->size);

    if (c < L->size)
        FatalError("W_ReadLump: only read %i of %i on lump %i", c, L->size, lump);
}

//
// LoadLumpIntoMemory
//
// Returns a copy of the lump (it is your responsibility to free it)
//
uint8_t *LoadLumpIntoMemory(int lump, int *length)
{
    int w_length = GetLumpLength(lump);

    if (length != nullptr)
        *length = w_length;

    uint8_t *data = new uint8_t[w_length + 1];

    W_RawReadLump(lump, data);

    // zero-terminate, handy for text parsers
    data[w_length] = 0;

    return data;
}

uint8_t *LoadLumpIntoMemory(const char *name, int *length)
{
    return LoadLumpIntoMemory(GetLumpNumberForName(name), length);
}

std::string LoadLumpAsString(int lump)
{
    // WISH: optimise this to remove temporary buffer
    int      length;
    uint8_t *data = LoadLumpIntoMemory(lump, &length);

    std::string result((char *)data, length);

    delete[] data;

    return result;
}

std::string LoadLumpAsString(const char *name)
{
    return LoadLumpAsString(GetLumpNumberForName(name));
}

//
// GetLumpNameFromIndex
//
const char *GetLumpNameFromIndex(int lump)
{
    return lump_info[lump].name;
}

// IsLumpInPwad
//
// check if a lump is in a pwad
//
// Returns true if found
bool IsLumpInPwad(const char *name)
{
    if (!name)
        return false;

    // first check images.ddf
    const Image *tempImage;

    tempImage = ImageLookup(name);
    if (tempImage)
    {
        if (tempImage->source_type_ == kImageSourceUser) // from images.ddf
        {
            return true;
        }
    }

    // if we're here then check pwad lumps
    int  lumpnum = CheckLumpNumberForName(name);
    int  filenum = -1;
    bool in_pwad = false;

    if (lumpnum != -1)
    {
        filenum = GetDataFileIndexForLump(lumpnum);

        if (filenum >= 2) // ignore edge_defs and the IWAD itself
        {
            DataFile *df = data_files[filenum];

            // we only want pwads
            if (df->kind_ == kFileKindPWAD || df->kind_ == kFileKindPackWAD)
            {
                in_pwad = true;
            }
        }
    }

    if (!in_pwad) // Check EPKs/folders now
    {
        // search from newest file to oldest
        for (int i = (int)data_files.size() - 1; i >= 2; i--) // ignore edge_defs and the IWAD itself
        {
            DataFile *df = data_files[i];
            if (df->kind_ == kFileKindFolder || df->kind_ == kFileKindEFolder || df->kind_ == kFileKindEPK ||
                df->kind_ == kFileKindEEPK)
            {
                if (FindStemInPack(df->pack_, name))
                {
                    in_pwad = true;
                    break;
                }
            }
        }
    }

    return in_pwad;
}

// IsLumpInAnyWad
//
// check if a lump is in any wad/epk at all
//
// Returns true if found
bool IsLumpInAnyWad(const char *name)
{
    if (!name)
        return false;

    int  lumpnum   = CheckLumpNumberForName(name);
    bool in_anywad = false;

    if (lumpnum != -1)
        in_anywad = true;

    if (!in_anywad)
    {
        // search from oldest to newest
        for (int i = 0; i < (int)data_files.size() - 1; i++)
        {
            DataFile *df = data_files[i];
            if (df->kind_ == kFileKindFolder || df->kind_ == kFileKindEFolder || df->kind_ == kFileKindEPK ||
                df->kind_ == kFileKindEEPK || df->kind_ == kFileKindIFolder || df->kind_ == kFileKindIPK)
            {
                if (FindStemInPack(df->pack_, name))
                {
                    in_anywad = true;
                    break;
                }
            }
        }
    }

    return in_anywad;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
