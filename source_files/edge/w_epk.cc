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

#include <algorithm>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "bsp.h"
#include "ddf_colormap.h"
#include "ddf_main.h"
#include "dm_state.h"
#include "epi.h"
#include "epi_filesystem.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "r_image.h"
#include "snd_types.h"
#include "w_files.h"

static std::unordered_set<std::string, epi::ContainerStringHash> blacklisted_directories = 
{".git", ".github", ".vscode", "autoload", "build", "cache", "cmake", "docs", "edge_defs", "libraries", "savegame", "screenshot", "scripts", "soundfont", "source_files"};

static constexpr const char *known_image_directories[5] = {"flats", "graphics", "skins", "textures", "sprites"};

static constexpr uint8_t kMaximumRecurseDepth = 10;

static uint8_t recurse_counter = 0;
class PackEntry
{
  public:
    // base filename
    std::string name_;

    // path relative to the PHYSFS root
    std::string pack_path_;

    PackEntry(std::string_view name, std::string_view ppath)
        : name_(name), pack_path_(ppath)
    {
    }

    ~PackEntry()
    {
    }

    bool operator==(std::string_view other) const
    {
        return (epi::StringCaseCompareASCII(name_, other) == 0);
    }
};

class PackDirectory
{
  public:
    std::string             name_;
    std::vector<PackEntry> entries_;

    PackDirectory(std::string_view name) : name_(name), entries_()
    {
    }

    ~PackDirectory()
    {
    }

    void AddEntry(std::string_view name, std::string_view ppath)
    {
        // check if already there
        for (size_t i = 0; i < entries_.size(); i++)
        {
            if (entries_[i] == name)
                return;
        }

        entries_.push_back(PackEntry(name, ppath));
    }

    bool operator==(std::string_view other) const
    {
        return (epi::StringCaseCompareASCII(name_, other) == 0);
    }
};

static std::vector<PackDirectory> search_directories;

// This consist of stems and their associated pack paths. This is used 
// during file look ups to quickly determine if a file is even present
// in the load path. It is a multimap because we have no qualms about the
// same stem/filename in multiple directories
static std::unordered_multimap<std::string, std::string, epi::ContainerStringHash> search_files;

static int FindDirectory(std::string_view name)
{
    EPI_ASSERT(!name.empty());
    int ret = -1;
    for (int i = 0, i_end = search_directories.size(); i < i_end; i++)
    {
        if (search_directories[i] == name)
        {
            ret = i;
            break;
        }
    }
    return ret;
}

//----------------------------------------------------------------------------
//  GENERAL STUFF
//----------------------------------------------------------------------------

// Generate automatic DDF for sound effects. This should happen prior to DDF processing
// so that DDFSFX entries can override them.
static void ProcessSounds()
{
    char **got_names = PHYSFS_enumerateFiles("sounds");

    // seems this only happens on out-of-memory error
    if (!got_names)
        FatalError("ProcessSounds: %s\n", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));

    char **p;
    PHYSFS_Stat statter;
    std::string text = "<SOUNDS>\n\n";

    for (p = got_names; *p; p++)
    {
        std::string pack_path = epi::PathAppend("sounds", *p);

        if (PHYSFS_stat(pack_path.c_str(), &statter) == 0)
        {
            LogPrint("Could not stat %s: %s\n", pack_path.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            continue;
        }

        if (statter.filetype == PHYSFS_FILETYPE_REGULAR)
        {
            if (SoundFilenameToFormat(*p) != kSoundUnknown)
            {
                std::string sfxname = epi::GetStem(*p);
                epi::StringUpperASCII(sfxname);
                // generate DDF for it...
                text += "[";
                text += sfxname;
                text += "]\n";

                text += "PACK_NAME = \"";
                text += *p;
                text += "\";\n";

                text += "PRIORITY  = 64;\n";
                text += "\n";
            }
        }
    }

    DDFAddFile(kDDFTypeSFX, text);

    PHYSFS_freeList(got_names);
}

// For now, this is just DDF/LDF until Lua is brought back - Dasho
static void ProcessScripts()
{
    char **got_names = PHYSFS_enumerateFiles("scripts");

    // seems this only happens on out-of-memory error
    if (!got_names)
        FatalError("ProcessScripts: %s\n", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));

    char **p;
    PHYSFS_Stat statter;

    for (p = got_names; *p; p++)
    {
        std::string pack_path = epi::PathAppend("scripts", *p);

        if (PHYSFS_stat(pack_path.c_str(), &statter) == 0)
        {
            LogPrint("Could not stat %s: %s\n", pack_path.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            continue;
        }

        if (statter.filetype == PHYSFS_FILETYPE_REGULAR)
        {
            DDFType type = DDFFilenameToType(*p);

            if (type != kDDFTypeUnknown)
            {
                epi::File *f = epi::FileOpen(pack_path.c_str(), epi::kFileAccessRead);
                if (!f)
                {
                    LogPrint("Could not read %s: %s\n", *p, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
                    continue;
                }

                std::string data = f->ReadAsString();

                delete f;

                DDFAddFile(type, data);
                continue;
            }
        }
    }

    PHYSFS_freeList(got_names);
}

// Only directories at the top level (i.e., immediately under root)
// are considered directories for our purposes. Everything else is an 
// entry within said directories, including subfolders and their contents.
// This makes it easier to reason about things like the "graphics", "sounds",
// "music" directories, etc
static void BuildDirectoryList()
{
    // Just in case, but we don't do live restarts (yet?)
    search_directories.clear();

    // Push the root directory first
    search_directories.push_back({"/"});

    char **got_names = PHYSFS_enumerateFiles("/");

    // seems this only happens on out-of-memory error
    if (!got_names)
        FatalError("BuildDirectoryList: %s\n", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));

    char **p;
    PHYSFS_Stat statter;

    for (p = got_names; *p; p++)
    {
        if (PHYSFS_stat(epi::PathAppend("/", *p).c_str(), &statter) == 0)
        {
            LogPrint("Could not stat %s: %s\n", *p, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            continue;
        }

        // Do not push certain directories ("scripts" and other known folders)
        if (statter.filetype == PHYSFS_FILETYPE_DIRECTORY)
        {
            if (blacklisted_directories.count(*p))
                continue;
            search_directories.push_back({*p});
        }
    }

    PHYSFS_freeList(got_names);
}

static void RescurseDirectory(const std::string &directory, std::vector<std::string> &entries)
{
    char **got_names = PHYSFS_enumerateFiles(directory.c_str());

    // seems this only happens on out-of-memory error
    if (!got_names)
        FatalError("RecurseDirectory: %s\n", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));

    char **p;
    PHYSFS_Stat statter;

    for (p = got_names; *p; p++)
    {
        std::string pack_path = epi::PathAppend(directory, *p);

        if (PHYSFS_stat(pack_path.c_str(), &statter) == 0)
        {
            LogPrint("Could not stat %s: %s\n", pack_path.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            continue;
        }

        if (statter.filetype == PHYSFS_FILETYPE_DIRECTORY)
        {
            if (recurse_counter == kMaximumRecurseDepth)
            {
                LogPrint("RecurseDirectory: Maximum depth reached; cannot read %s\n", pack_path.c_str());
                continue;
            }
            else
            {
                if (blacklisted_directories.count(*p))
                    continue;
                recurse_counter++;
                RescurseDirectory(pack_path, entries);
            }
        }
        else if (statter.filetype == PHYSFS_FILETYPE_REGULAR)
        {
            entries.push_back(pack_path);
        }
    }

    if (recurse_counter > 0)
        recurse_counter--;

    PHYSFS_freeList(got_names);
}

static void BuildEntryList()
{
    // Just in case, but we don't do live restarts (yet?)
    search_files.clear();

    for (PackDirectory &dir : search_directories)
    {
        char **got_names = PHYSFS_enumerateFiles(dir.name_.c_str());

        // seems this only happens on out-of-memory error
        if (!got_names)
            FatalError("BuildEntryList: %s\n", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));

        char **p;
        PHYSFS_Stat statter;

        for (p = got_names; *p; p++)
        {
            std::string pack_path = epi::PathAppend(dir.name_, *p);

            if (PHYSFS_stat(pack_path.c_str(), &statter) == 0)
            {
                LogPrint("Could not stat %s: %s\n", pack_path.c_str(), PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
                continue;
            }

            if (statter.filetype == PHYSFS_FILETYPE_DIRECTORY)
            {
                if (blacklisted_directories.count(*p))
                    continue;
                std::vector<std::string> entries;
                RescurseDirectory(pack_path, entries);
                for (const std::string &entry : entries)
                {
                    dir.AddEntry(epi::GetFilename(entry), entry);
                    std::string stem = epi::GetStem(entry);
                    epi::StringUpperASCII(stem);
                    search_files.insert({stem, entry});
                }
            }
            else if (statter.filetype == PHYSFS_FILETYPE_REGULAR)
            {
                dir.AddEntry(*p, pack_path);
                std::string stem = epi::GetStem(*p);
                epi::StringUpperASCII(stem);
                search_files.insert({stem, pack_path});
            }
        }

        PHYSFS_freeList(got_names);
    }
}

void ProcessPackContents()
{
    // First, we build the directory, entry, and search lists, then we perform certain
    // actions for select directories
    BuildDirectoryList();
    BuildEntryList();

    int d = -1;
    for (std::string_view dir_name : known_image_directories)
    {
        d = FindDirectory(dir_name);
        if (d < 0)
            continue;
        for (size_t i = 0; i < search_directories[d].entries_.size(); i++)
        {
            PackEntry &entry = search_directories[d].entries_[i];

            // split filename in stem + extension
            std::string stem = epi::GetStem(entry.name_);
            std::string ext  = epi::GetExtension(entry.name_);

            epi::StringLowerASCII(ext);

            if (ext == ".png")
            {
                std::string texname;

                epi::TextureNameFromFilename(texname, stem);

                LogDebug("- Adding image file in EPK: %s\n", entry.pack_path_.c_str());

                if (dir_name == "textures")
                    AddPackImageSmart(texname, kImageSourceGraphic, entry.pack_path_, real_textures);
                else if (dir_name == "graphics")
                    AddPackImageSmart(texname, kImageSourceGraphic, entry.pack_path_, real_graphics);
                else if (dir_name == "flats")
                    AddPackImageSmart(texname, kImageSourceGraphic, entry.pack_path_, real_flats);
                else if (dir_name == "skins") // Not sure about this still
                    AddPackImageSmart(texname, kImageSourceSprite, entry.pack_path_, real_sprites);
            }
            else
            {
                LogWarning("Unknown image type in EPK: %s\n", entry.name_.c_str());
            }
        }
    }

    // Build nodes if not already present for any text files in the /maps directory
    d = FindDirectory("maps");
    if (d > 0)
    {
        for (size_t i = 0; i < search_directories[d].entries_.size(); i++)
        {
            PackEntry &entry = search_directories[d].entries_[i];
            if (epi::StringCaseCompareASCII(epi::GetExtension(entry.pack_path_), ".txt") == 0)
            {
                uint64_t udmf_hash;
                epi::File *udmf_file = epi::FileOpen(entry.pack_path_, epi::kFileAccessRead);
                if (!udmf_file)
                    FatalError("Error opening %s\n", entry.pack_path_.c_str());
                std::string udmf_string = udmf_file->ReadAsString();
                delete udmf_file;
                udmf_hash = epi::StringHash64(udmf_string);
                std::string node_file = epi::PathAppend("cache", epi::StringFormat("%s-%lu.xgl",epi::GetStem(entry.pack_path_).c_str(), udmf_hash));
                if (!epi::FileExists(node_file))
                    ajbsp::BuildLevel(epi::GetStem(entry.pack_path_), node_file, udmf_string);
            }
        }
    }
}

bool CheckPackFile(const std::string &name, std::string_view check_dirs)
{
    EPI_ASSERT(!name.empty());

    std::string check_stem = epi::GetStem(name);
    epi::StringUpperASCII(check_stem);

    // quick file stem check to see if it's present at all
    if (!search_files.count(check_stem))
        return false;

    // Specific path given; attempt to find as-is, otherwise return false
    if (name != epi::GetFilename(name))
    {
        return epi::FileExists(name);
    }
    // If open_dirs is empty, find the first matching filename. No guarantee on which is found first if there are
    // multiple. check_dirs should be populated with directory names if wanting to narrow it down
    else if (check_dirs.empty())
    {
        auto results = search_files.equal_range(check_stem);
        for (auto file = results.first; file != results.second; ++file)
        {
            if (epi::StringCaseCompareASCII(name, epi::GetFilename(file->second)) == 0)
                return true;
        }
        return false;
    }
    // A list of one or more acceptable directories was passed in, if there is no matching
    // file in any of them (or the directories don't exist) return false
    else
    {
        std::vector<std::string> dirs = epi::SeparatedStringVector(check_dirs, ',');
        int d = -1;
        for (std::string_view dir : dirs)
        {
            d = FindDirectory(dir);
            if (d < 0)
                continue;
            for (const PackEntry &entry : search_directories[d].entries_)
            {
                if (entry == name)
                    return true;
            }
        }
        return false;
    }

    // Fallback
    return false;
}

epi::File *OpenPackFile(const std::string &name, std::string_view open_dirs)
{
    // when file does not exist, this returns nullptr.
    EPI_ASSERT(!name.empty());

    std::string open_stem = epi::GetStem(name);
    epi::StringUpperASCII(open_stem);

    // quick file stem check to see if it's present at all
    if (!search_files.count(open_stem))
        return nullptr;

    // Specific path given; attempt to open as-is, otherwise return nullptr
    if (name != epi::GetFilename(name))
    {
        return epi::FileOpen(name, epi::kFileAccessRead);
    }
    // If open_dirs is empty, return the first matching filename. No guarantee on which is returned if there are
    // multiple. open_dirs should be populated with directory names if wanting to narrow it down
    else if (open_dirs.empty())
    {
        auto results = search_files.equal_range(open_stem);
        for (auto file = results.first; file != results.second; ++file)
        {
            if (epi::StringCaseCompareASCII(name, epi::GetFilename(file->second)) == 0)
                return epi::FileOpen(file->second, epi::kFileAccessRead);
        }
        return nullptr;
    }
    // A list of one or more acceptable directories was passed in, if there is no matching
    // file in any of them (or the directories don't exist) return nullptr
    else
    {
        std::vector<std::string> dirs = epi::SeparatedStringVector(open_dirs, ',');
        int d = -1;
        for (const std::string &dir : dirs)
        {
            d = FindDirectory(dir);
            if (d < 0)
                continue;
            for (const PackEntry &entry : search_directories[d].entries_)
            {
                if (entry == name)
                    return epi::FileOpen(entry.pack_path_, epi::kFileAccessRead);
            }
        }
        return nullptr;
    }

    // Fallback
    return nullptr;
}

std::vector<std::string> GetPackSpriteList()
{
    std::vector<std::string> found_sprites;

    int d = FindDirectory("sprites");
    if (d > 0)
    {
        for (size_t i = 0; i < search_directories[d].entries_.size(); i++)
        {
            PackEntry &entry = search_directories[d].entries_[i];

            // split filename in stem + extension
            std::string stem = epi::GetStem(entry.name_);
            std::string ext  = epi::GetExtension(entry.name_);

            epi::StringLowerASCII(ext);

            if (ext == ".png")
            {
                std::string texname;
                epi::TextureNameFromFilename(texname, stem);

                bool addme = true;
                // Don't add things already defined in DDFIMAGE
                for (ImageDefinition *img : imagedefs)
                {
                    if (epi::StringCaseCompareASCII(img->name_, texname) == 0)
                    {
                        addme = false;
                        break;
                    }
                }
                if (addme)
                    found_sprites.push_back(entry.pack_path_);
            }
        }
    }

    return found_sprites;
}

void ProcessAllInPack(const std::string &df)
{
    // Mount to PHYSFS root
    if (PHYSFS_mount(df.c_str(), "/", 0) == 0)
        FatalError("ProcessAllInPack: Failed to mount %s!\n", df.c_str());

    // We need to process sounds and scripts as they are loaded to allow for templating/overrides; everything else
    // can (and should) use the normal PHYSFS search paths once everything is loaded.
    // DDFAddFile does check hashes so should prevent the same files being added for processing multiple
    // times if they don't change when mounting new folders/archives
    ProcessSounds();
    ProcessScripts();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
