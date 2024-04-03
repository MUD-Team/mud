// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 832e2329275a3f822dacfe61930444aad697042e $
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
//	File Input/Output Operations for windows-like platforms.
//
//-----------------------------------------------------------------------------

#if defined(_WIN32)

#if defined(UNIX)
#error "_WIN32 is mutually exclusive with UNIX"
#endif

#include <shlobj.h>
#include <shlwapi.h>

#include "cmdlib.h"
#include "i_system.h"
#include "m_fileio.h"
#include "m_ostring.h"
#include "odamex.h"
#include "w_wad.h"
#include "win32inc.h"

std::string M_GetUserFileName(const std::string &file)
{
    std::string path = file;
    // If an absolute path that contains our write directory,
    // make it relative
    // Todo: Should we do case insensitive compare on Windows? - Dasho
    std::string::size_type pos = file.find(M_GetWriteDir());
    if (pos != std::string::npos)
    {
        path = file.substr(pos+M_GetWriteDir().size());
    }

    // Still an absolute path?  If so, stop here.
    if (!PathIsRelative(path.c_str()))
    {
        I_FatalError("Attempting to write to %s, which is outside of the write directory at %s\n", file.c_str(), M_GetWriteDir().c_str());
    }

    // If we get here, it should just be writing somewhere in
    // our PHYSFS write path
    return path;
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
    std::string all_ext = dir + PATHSEP "*";
    // all_ext += ext;

    WIN32_FIND_DATA FindFileData;
    HANDLE          hFind = FindFirstFile(all_ext.c_str(), &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
        return "";

    std::string                    found;
    std::vector<OString>::iterator found_it = cmp_files.end();
    do
    {
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        // Not only find a match, but check if it is a better match than we
        // found previously.
        OString                        check   = StdStringToUpper(FindFileData.cFileName);
        std::vector<OString>::iterator this_it = std::find(cmp_files.begin(), cmp_files.end(), check);
        if (this_it < found_it)
        {
            const std::string local_file(dir + PATHSEP + FindFileData.cFileName);
            const OMD5Hash    local_hash = W_MD5(local_file);

            if (hash.empty() || hash == local_hash)
            {
                // Found a match.
                found    = FindFileData.cFileName;
                found_it = this_it;
                if (found_it == cmp_files.begin())
                {
                    // Found the best possible match, we're done.
                    break;
                }
            }
            else if (!hash.empty())
            {
                Printf(PRINT_WARNING, "WAD at %s does not match required copy\n", local_file.c_str());
                Printf(PRINT_WARNING, "Local MD5: %s\n", local_hash.getHexCStr());
                Printf(PRINT_WARNING, "Required MD5: %s\n\n", hash.getHexCStr());
            }
        }
    } while (FindNextFile(hFind, &FindFileData));

    FindClose(hFind);
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

    const std::string all_ext = dir + PATHSEP "*";

    WIN32_FIND_DATA FindFileData;
    HANDLE          hFind = FindFirstFile(all_ext.c_str(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return rvo;
    }

    do
    {
        // Skip directories.
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        // Find the file.
        std::string                    check = StdStringToUpper(FindFileData.cFileName);
        std::vector<OString>::iterator it    = std::find(files.begin(), files.end(), check);

        if (it == files.end())
            continue;

        rvo.push_back(check);
    } while (FindNextFile(hFind, &FindFileData));

    return rvo;
}

// Scan for PWADs and DEH and BEX files
std::vector<std::string> M_PWADFilesScanDir(std::string dir)
{
    std::vector<std::string> rvo;

    // Fix up parameters.
    dir = M_CleanPath(dir);

    const std::string all_ext = dir + PATHSEP "*";

    WIN32_FIND_DATA FindFileData;
    HANDLE          hFind = FindFirstFile(all_ext.c_str(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return rvo;
    }

    do
    {
        // Skip directories.
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        std::string filename = std::string(FindFileData.cFileName);

        // Don't care about files with names shorter than the extension
        if (filename.length() < 4)
            continue;

        // Only return files with correct extensions
        std::string check = StdStringToUpper(filename).substr(filename.length() - 4);
        if (check.compare(".WAD") && check.compare(".DEH") && check.compare(".BEX"))
            continue;

        rvo.push_back(filename);
    } while (FindNextFile(hFind, &FindFileData));

    return rvo;
}

bool M_GetAbsPath(const std::string &path, std::string &out)
{
    TCHAR  buffer[MAX_PATH];
    LPSTR *file_part = NULL;
    DWORD  wrote     = GetFullPathNameA(path.c_str(), ARRAY_LENGTH(buffer), buffer, file_part);
    if (wrote == 0)
    {
        return false;
    }

    out = buffer;
    return true;
}

#endif
