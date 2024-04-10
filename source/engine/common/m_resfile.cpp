// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: a278c769adaf8d89c56cf5a8c557fb394298d75f $
//
// Copyright (C) 2021 by Alex Mayfield.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  A handle that wraps a resolved file on disk.
//
//-----------------------------------------------------------------------------

#include "m_resfile.h"

#include <algorithm>

#include "c_dispatch.h"
#include "cmdlib.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "md5.h"
#include "odamex.h"
#include "w_ident.h"
#include "w_wad.h"

static constexpr uint8_t ZipMagic[4] = { 0x50, 0x4b, 0x03, 0x04 };

/**
 * @brief Populate an OResFile.
 *
 * @param out OResFile to populate.
 * @param file Complete and working path to file to populate with.
 * @return True if the OResFile was populated successfully.
 */
bool OResFile::make(OResFile &out, const std::string &file)
{
    std::string fullpath = file;

    std::string basename = M_ExtractFileName(fullpath);
    if (basename.empty())
        basename = file;

    std::string mountpath;
    M_ExtractFilePath(fullpath, mountpath);
    if (!mountpath.empty())
    {
        PHYSFS_mount(mountpath.c_str(), NULL, 0);
    }

    if (!M_FileExists(basename))
    {
        return false;
    }

    // Stat and assign OFILE type (if valid)
    PHYSFS_Stat stat_check;
    ofile_t type;
    if (PHYSFS_stat(basename.c_str(), &stat_check) == 0)
        return false;
    else if (stat_check.filetype == PHYSFS_FILETYPE_REGULAR)
    {
        // check for archive
        PHYSFS_File *zipcheck = PHYSFS_openRead(basename.c_str());
        if (!zipcheck) // something's wrong
            return false;
        if (PHYSFS_fileLength(zipcheck) < 4)
            type = OFILE_LOOSE;
        else
        {
            uint8_t magic[4] = {0, 0, 0, 0};
            if (PHYSFS_readBytes(zipcheck, magic, 4) != 4)  // something's wrong
            {
                PHYSFS_close(zipcheck);
                return false;
            }
            if (memcmp(magic, ZipMagic, 4) != 0)
                type = OFILE_LOOSE; // Not a zip, assume it's just a regular file
            else
                type = OFILE_ARCHIVE;
        }
        PHYSFS_close(zipcheck);
    }
    else if (stat_check.filetype == PHYSFS_FILETYPE_DIRECTORY)
        type = OFILE_FOLDER;
    else // symlink or device or something weird
        return false;

    OMD5Hash hash;
    if (type != OFILE_FOLDER)
    {
        hash = W_MD5(basename);
        if (hash.empty())
        {
            return false;
        }
    }

    out.m_fullpath = fullpath;
    out.m_type     = type;
    out.m_md5      = hash;
    out.m_basename = basename;
    return true;
}

/**
 * @brief Populate an OResFile with an already calculated hash.
 *
 * @param out OResFile to populate.
 * @param file Complete and working path to file to populate with.
 * @param hash Correct hash of file to populate with.  This is not checked,
 *             and should only be used if you have already hashed the passed
 *             file.
 * @return True if the OResFile was populated successfully.
 */
bool OResFile::makeWithHash(OResFile &out, const std::string &file, const OMD5Hash &hash)
{
    std::string fullpath = file;

    if (hash.empty())
    {
        return false;
    }

    std::string basename = M_ExtractFileName(fullpath);
    // This will need to change as mounting directories is fleshed out - Dasho
    if (basename.empty())
    {
        return false;
    }

    std::string mountpath;
    M_ExtractFilePath(fullpath, mountpath);
    if (!mountpath.empty())
    {
        PHYSFS_mount(mountpath.c_str(), NULL, 0);
    }

    if (!M_FileExists(basename))
    {
        return false;
    }

    // Stat and assign OFILE type (if valid)
    PHYSFS_Stat stat_check;
    ofile_t type;
    if (PHYSFS_stat(basename.c_str(), &stat_check) == 0)
        return false;
    else if (stat_check.filetype == PHYSFS_FILETYPE_REGULAR)
    {
        // check for archive
        PHYSFS_File *zipcheck = PHYSFS_openRead(basename.c_str());
        if (!zipcheck) // something's wrong
            return false;
        if (PHYSFS_fileLength(zipcheck) < 4)
            type = OFILE_LOOSE;
        else
        {
            uint8_t magic[4] = {0, 0, 0, 0};
            if (PHYSFS_readBytes(zipcheck, magic, 4) != 4)  // something's wrong
            {
                PHYSFS_close(zipcheck);
                return false;
            }
            if (memcmp(magic, ZipMagic, 4) != 0)
                type = OFILE_LOOSE; // Not a zip, assume it's just a regular file
            else
                type = OFILE_ARCHIVE;
        }
        PHYSFS_close(zipcheck);
    }
    else if (stat_check.filetype == PHYSFS_FILETYPE_DIRECTORY)
        type = OFILE_FOLDER;
    else // symlink or device or something weird
        return false;

    out.m_fullpath = fullpath;
    out.m_type     = type;
    out.m_md5      = hash;
    out.m_basename = basename;
    return true;
}

/**
 * @brief Populate an OWantFile.
 *
 * @param out OWantFile to populate.
 * @param file Path fragment to file to populate with that may or may not exist.
 * @return True if the OWantFile was populated successfully.
 */
bool OWantFile::make(OWantFile &out, const std::string &file)
{
    std::string basename = M_ExtractFileName(file);
    // This will need to change as mounting directories is fleshed out - Dasho
    if (basename.empty())
    {
        return false;
    }

    std::string extension;
    M_ExtractFileExtension(basename, extension);

    out.m_wantedpath = file;
    out.m_basename   = basename;
    out.m_extension  = std::string(".") + extension;
    return true;
}

/**
 * @brief Populate an OResFile with a suggested hash.
 *
 * @param out OWantFile to populate.
 * @param file Path fragment to file to populate with that may or may not exist.
 * @param hash Desired hash to populate with.
 * @return True if the OWantFile was populated successfully.
 */
bool OWantFile::makeWithHash(OWantFile &out, const std::string &file, const OMD5Hash &hash)
{
    std::string basename = M_ExtractFileName(file);
    if (basename.empty())
    {
        return false;
    }

    std::string extension;
    M_ExtractFileExtension(basename, extension);

    out.m_wantedpath = file;
    out.m_wantedMD5  = hash;
    out.m_basename   = basename;
    out.m_extension  = extension;
    return true;
}

/**
 * @brief Turn passed list of ResFiles to string.
 *
 * @param files Files to stringify.
 */
std::string M_ResFilesToString(const OResFiles &files)
{
    std::vector<std::string> strings;
    strings.reserve(files.size());
    for (OResFiles::const_iterator it = files.begin(); it != files.end(); ++it)
    {
        strings.push_back(it->getBasename());
    }
    return JoinStrings(strings, ", ");
}

/**
 * @brief Resolve an OResFile given a filename.
 *
 * @param out Output OResFile.  On error, this object is not touched.
 * @param wanted Wanted file to resolve.
 * @return True if the file was resolved, otherwise false.
 */
bool M_ResolveWantedFile(OResFile &out, const OWantFile &wanted)
{
    std::string mountpath;
    M_ExtractFilePath(wanted.getWantedPath(), mountpath);
    if (!mountpath.empty())
    {
        PHYSFS_mount(mountpath.c_str(), NULL, 0);
    }

    // If someone goes throught the effort of pointing directly to a file
    // correctly, believe them.
    if (M_FileExists(wanted.getBasename()))
    {
        if (wanted.getWantedMD5().empty())
        {
            // No hash preference.
            return OResFile::make(out, wanted.getWantedPath());
        }

        OMD5Hash hash = W_MD5(wanted.getBasename());
        if (wanted.getWantedMD5() == hash)
        {
            // File matches our hash.
            return OResFile::makeWithHash(out, wanted.getWantedPath(), hash);
        }
        else
            return false;
    }

    // Couldn't find anything.
    return false;
}

std::string M_GetCurrentWadHashes()
{
    std::string builder = "";

    for (OResFiles::const_iterator it = ::wadfiles.begin(); it != ::wadfiles.end(); ++it)
    {
        std::string base = it->getBasename().c_str();
        std::string hash = it->getMD5().getHexCStr();
        std::string line = base + ',' + hash + '\n';

        builder += line;
    }

    return builder;
}

BEGIN_COMMAND(whereis)
{
    if (argc < 2)
        return;

    OWantFile want;
    OWantFile::make(want, argv[1]);

    OResFile res;
    if (M_ResolveWantedFile(res, want))
    {
        Printf("basename: %s\nfullpath: %s\nCRC32: %s\nMD5: %s\n", res.getBasename().c_str(), res.getFullpath().c_str(),
               W_CRC32(res.getFullpath()).getHexCStr(), res.getMD5().getHexCStr());
        return;
    }

    Printf("Could not find location of \"%s\".\n", argv[1]);
}
END_COMMAND(whereis)

BEGIN_COMMAND(loaded)
{
    for (OResFiles::const_iterator it = ::wadfiles.begin(); it != ::wadfiles.end(); ++it)
    {
        Printf("%s\n", it->getBasename().c_str());
        Printf("  PATH: %s\n", it->getFullpath().c_str());
        Printf("  MD5:  %s\n", it->getMD5().getHexCStr());
    }
}
END_COMMAND(loaded)