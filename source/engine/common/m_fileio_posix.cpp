// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 6bff3fd2641871081757768a28707483333ab0a7 $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//	File Input/Output Operations
//
//-----------------------------------------------------------------------------

#if defined(UNIX)

#if defined(_WIN32)
#error "UNIX is mutually exclusive with _WIN32"
#endif

#if defined(__linux__)
#include <linux/limits.h>
#else
#include <sys/syslimits.h>
#endif
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sstream>

#include "cmdlib.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_ostring.h"
#include "odamex.h"
#include "w_wad.h"

std::string M_GetUserFileName(const std::string &file)
{
    std::string path = file;
    // If an absolute path that contains our write directory,
    // make it relative.
    std::string::size_type pos = file.find(M_GetWriteDir());
    if (pos != std::string::npos)
    {
        path = file.substr(pos+M_GetWriteDir().size());
    }
    // Still an absolute path?  If so, stop here.
    size_t fileLen = path.length();
    if (fileLen >= 1 && M_IsPathSep(path[0]))
    {
        I_FatalError("Attempting to write to %s, which is outside of the write directory at %s\n", file.c_str(), M_GetWriteDir().c_str());
    }

    // If we get here, it should just be writing somewhere in
    // our PHYSFS write path
    return M_CleanPath(path);
}

std::string M_BaseFileSearchDir(std::string dir, const std::string &name, const std::vector<std::string> &exts,
                                const OMD5Hash &hash)
{
    dir = M_CleanPath(dir);
    std::vector<OString> cmp_files;
    for (std::vector<std::string>::const_iterator it = exts.begin(); it != exts.end(); ++it)
    {
        if (!hash.empty())
        {
            // Filenames with supplied hashes always match first.
            cmp_files.push_back(StdStringToUpper(name + "." + hash.getHexStr().substr(0, 6) + *it));
        }
        cmp_files.push_back(StdStringToUpper(name + *it));
    }

    // denis - list files in the directory of interest, case-desensitize
    // then see if wanted wad is listed
    struct dirent **namelist = 0;
    int             n        = scandir(dir.c_str(), &namelist, 0, alphasort);

    std::string                    found;
    std::vector<OString>::iterator found_it = cmp_files.end();
    for (int i = 0; i < n && namelist[i]; i++)
    {
        const std::string d_name = namelist[i]->d_name;
        M_Free(namelist[i]);

        if (found_it == cmp_files.begin())
            continue;

        if (d_name == "." || d_name == "..")
            continue;

        const std::string              check   = StdStringToUpper(d_name);
        std::vector<OString>::iterator this_it = std::find(cmp_files.begin(), cmp_files.end(), check);
        if (this_it < found_it)
        {
            const std::string local_file(dir + PATHSEP + d_name);
            const OMD5Hash    local_hash(W_MD5(local_file));

            if (hash.empty() || hash == local_hash)
            {
                // Found a match.
                found    = d_name;
                found_it = this_it;
                continue;
            }
            else if (!hash.empty())
            {
                Printf(PRINT_WARNING, "WAD at %s does not match required copy\n", local_file.c_str());
                Printf(PRINT_WARNING, "Local MD5: %s\n", local_hash.getHexCStr());
                Printf(PRINT_WARNING, "Required MD5: %s\n\n", hash.getHexCStr());
            }
        }
    }

    M_Free(namelist);
    return found;
}

std::vector<std::string> M_BaseFilesScanDir(std::string dir, std::vector<OString> files)
{
    std::vector<std::string> rvo;

    // Fix up parameters.
    dir = M_CleanPath(dir);
    for (size_t i = 0; i < files.size(); i++)
    {
        files[i] = StdStringToUpper(files[i]);
    }

    struct dirent **namelist = 0;
    int             n        = scandir(dir.c_str(), &namelist, 0, alphasort);

    for (int i = 0; i < n && namelist[i]; i++)
    {
        const std::string d_name = namelist[i]->d_name;
        M_Free(namelist[i]);

        // Find the file.
        std::string                    check = StdStringToUpper(d_name);
        std::vector<OString>::iterator it    = std::find(files.begin(), files.end(), check);

        if (it == files.end())
            continue;

        rvo.push_back(d_name);
    }

    return rvo;
}

// Scan for PWADs and DEH and BEX files
std::vector<std::string> M_PWADFilesScanDir(std::string dir)
{
    std::vector<std::string> rvo;

    // Fix up parameters.
    dir = M_CleanPath(dir);

    struct dirent **namelist = 0;
    int             n        = scandir(dir.c_str(), &namelist, 0, alphasort);

    for (int i = 0; i < n && namelist[i]; i++)
    {
        const std::string d_name = namelist[i]->d_name;
        M_Free(namelist[i]);

        // Don't care about files with names shorter than the extensions
        if (d_name.length() < 4)
            continue;

        // Only return files with correct extensions
        std::string check = StdStringToUpper(d_name).substr(d_name.length() - 4);
        if (check.compare(".WAD") && check.compare(".DEH") && check.compare(".BEX"))
            continue;

        rvo.push_back(d_name);
    }

    return rvo;
}

bool M_GetAbsPath(const std::string &path, std::string &out)
{

    char  buffer[PATH_MAX];
    char *res = realpath(path.c_str(), buffer);
    if (res == NULL)
    {
        return false;
    }
    out = res;
    return true;
}

#endif
