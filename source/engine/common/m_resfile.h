// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ce72780ee95b287ec1d0700a84452a751ec9396f $
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
//  A handle that wraps a resolved file on disk.
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>

#include "ohash.h"

enum ofile_t
{
    /**
     * @brief Zips passed via -game or -file.
     */
    OFILE_ARCHIVE,

    /**
     * @brief (Real, not PHYSFS) directories passed via -game or -file.
     */
    OFILE_FOLDER,

    /**
     * @brief Individual files passed via -game or -file; unsure of utility for MUD yet - Dasho
     */
    OFILE_LOOSE,
};

/**
 * @brief A handle that wraps a resolved file on disk.
 */
class OResFile
{
    std::string m_fullpath;
    ofile_t     m_type;
    OMD5Hash    m_md5;
    std::string m_basename;

  public:
    /**
     * @brief Get the resource file type.
     */
    const ofile_t &getType() const
    {
        return m_type;
    }


    bool operator==(const OResFile &other) const
    {
        return m_md5 == other.m_md5;
    }
    bool operator!=(const OResFile &other) const
    {
        return !(operator==(other));
    }

    /**
     * @brief Get the full absolute path to the file.
     */
    const std::string &getFullpath() const
    {
        return m_fullpath;
    }

    /**
     * @brief Get the MD5 hash of the file.
     */
    const OMD5Hash &getMD5() const
    {
        return m_md5;
    }

    /**
     * @brief Get the base filename, with no path.
     */
    const std::string &getBasename() const
    {
        return m_basename;
    }

    static bool make(OResFile &out, const std::string &file);
    static bool makeWithHash(OResFile &out, const std::string &file, const OMD5Hash &hash);
};
typedef std::vector<OResFile> OResFiles;

/**
 * @brief A handle that represents a "wanted" file that may or may not exist.
 */
class OWantFile
{
    std::string m_wantedpath;
    OMD5Hash    m_wantedMD5;
    std::string m_basename;
    std::string m_extension;

  public:
    /**
     * @brief Get the original "wanted" path.
     */
    const std::string &getWantedPath() const
    {
        return m_wantedpath;
    }

    /**
     * @brief Get the assumed hash of the file, or an empty string if there
     *        is no hash.
     */
    const OMD5Hash &getWantedMD5() const
    {
        return m_wantedMD5;
    }

    /**
     * @brief Get the base filename of the resource, with no directory.
     */
    const std::string &getBasename() const
    {
        return m_basename;
    }

    /**
     * @brief Get the extension of the resource.
     */
    const std::string &getExt() const
    {
        return m_extension;
    }

    static bool make(OWantFile &out, const std::string &file);
    static bool makeWithHash(OWantFile &out, const std::string &file, const OMD5Hash &hash);
};
typedef std::vector<OWantFile> OWantFiles;

struct fileIdentifier_t;

struct scannedIWAD_t
{
    std::string             path;
    const fileIdentifier_t *id;
};

struct scannedPWAD_t
{
    std::string path;
    std::string filename;
};

std::string                     M_ResFilesToString(const OResFiles &files);
bool                            M_ResolveWantedFile(OResFile &out, const OWantFile &wanted);
