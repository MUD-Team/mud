//----------------------------------------------------------------------------
//  EDGE Filesystem Class
//----------------------------------------------------------------------------
//
//  Copyright (c) 2003-2024 The EDGE Team.
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

#include "epi.h"
#include "file.h"
#include "filesystem.h"
#include "epi_sdl.h"
#include "epi_windows.h"
#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

namespace epi
{

#ifdef _WIN32 // Windows API
static inline bool IsDirectorySeparator(const char c)
{
    return (c == '\\' || c == '/' || c == ':'); // Kester added ':'
}
bool FS_IsAbsolute(std::string_view path)
{
	SYS_ASSERT(!path.empty());

    // Check for Drive letter, colon and slash...
    if (path.size() > 2 && path[1] == ':' && 
        (path[2] == '\\' || path[2] == '/') && 
        isalpha(path[0]))
    {
        return true;
    }

    // Check for share name...
    if (path.size() > 1 && path[0] == '\\' && path[1] == '\\')
        return true;

    return false;
}
static const wchar_t *FlagsToANSIMode(int flags)
{
    // Must have some value in flags
    if (flags == 0)
        return nullptr;

    // Check for any invalid combinations
    if ((flags & kFileAccessWrite) && (flags & kFileAccessAppend))
        return nullptr;

    if (flags & kFileAccessRead)
    {
        if (flags & kFileAccessWrite)
            return L"wb+";
        else if (flags & kFileAccessAppend)
            return L"ab+";
        else
            return L"rb";
    }
    else
    {
        if (flags & kFileAccessWrite)
            return L"wb";
        else if (flags & kFileAccessAppend)
            return L"ab";
        else
            return nullptr; // Invalid
    }
}
FILE *FS_OpenRawFile(std::string_view name, unsigned int flags)
{
    const wchar_t *mode = FlagsToANSIMode(flags);
    if (!mode) return nullptr;
    std::wstring wname = epi::utf8_to_wstring(name);
    return _wfopen(wname.c_str(), mode);
}
bool FS_Delete(std::string_view name)
{
    SYS_ASSERT(!name.empty());
    std::wstring wname = epi::utf8_to_wstring(name);
    return _wremove(wname.c_str()) == 0;
}
bool FS_IsDir(std::string_view dir)
{
    SYS_ASSERT(!dir.empty());
    std::wstring wide_dir = epi::utf8_to_wstring(dir);
    struct _stat dircheck;
    if (_wstat(wide_dir.c_str(), &dircheck) != 0)
        return false;
    return(dircheck.st_mode & _S_IFDIR);
}
static std::string FS_GetCurrDir()
{
    std::string directory;
    const wchar_t *dir = _wgetcwd(nullptr, 0);
    if (dir)
        directory = epi::wstring_to_utf8(dir);
	return directory; // can be empty
}
bool FS_SetCurrDir(std::string_view dir)
{
    SYS_ASSERT(!dir.empty());
    std::wstring wdir = epi::utf8_to_wstring(dir);
    return _wchdir(wdir.c_str()) == 0;
}
bool FS_MakeDir(std::string_view dir)
{
    SYS_ASSERT(!dir.empty());
    std::wstring wdirectory = epi::utf8_to_wstring(dir);
    return _wmkdir(wdirectory.c_str()) == 0;
}
bool FS_Exists(std::string_view name)
{
    SYS_ASSERT(!name.empty());
    std::wstring wname = epi::utf8_to_wstring(name);
    return _waccess(wname.c_str(), 0) == 0;
}
bool FS_Access(std::string_view name)
{
    // The codebase only seems to use this to test read access, so we
    // shouldn't need to pass any modes as a parameter
    SYS_ASSERT(!name.empty());
    std::wstring wname = epi::utf8_to_wstring(name);
    if (_waccess(wname.c_str(), 4) == 0) // Read-only
        return true;
    else if (_waccess(wname.c_str(), 6) == 0) // Read/write
        return true;
    else
        return false;
}
bool FS_ReadDir(std::vector<dir_entry_c> &fsd, std::string &dir, const char *mask)
{
    if (dir.empty() || !FS_Exists(dir) || !mask)
        return false;

    std::string prev_dir = FS_GetCurrDir();

    if (prev_dir.empty()) // Something goofed up, don't make it worse
        return false;

    if (!FS_SetCurrDir(dir))
        return false;

    std::wstring fmask = epi::utf8_to_wstring(mask);
    WIN32_FIND_DATAW fdataw;
    HANDLE fhandle = FindFirstFileW(fmask.c_str(), &fdataw);

    if (fhandle == INVALID_HANDLE_VALUE)
	{
		FS_SetCurrDir(prev_dir);
		return false;
	}

    fsd.clear();

	do
	{
        std::string filename = epi::wstring_to_utf8(fdataw.cFileName);
	    if (filename == "." || filename == "..")
		{
		  // skip the funky "." and ".." dirs 
		}
		else
		{
            epi::dir_entry_c new_entry;
            new_entry.name = dir;
            new_entry.is_dir = (fdataw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
            new_entry.size = new_entry.is_dir ? 0 : fdataw.nFileSizeLow;
            new_entry.name.push_back('/');
            new_entry.name.append(filename);
            fsd.push_back(new_entry);
		}
	}
	while (FindNextFileW(fhandle, &fdataw));

	FindClose(fhandle);
	FS_SetCurrDir(prev_dir);
	return true;
}
bool FS_WalkDir(std::vector<dir_entry_c> &fsd, std::string &dir)
{
    if (dir.empty() || !FS_Exists(dir))
        return false;

    std::string prev_dir = FS_GetCurrDir();

    if (prev_dir.empty()) // Something goofed up, don't make it worse
        return false;

    if (!FS_SetCurrDir(dir))
        return false;

    WIN32_FIND_DATAW fdataw;
    HANDLE fhandle = FindFirstFileW(L"*.*", &fdataw);

    if (fhandle == INVALID_HANDLE_VALUE)
	{
		FS_SetCurrDir(prev_dir);
		return false;
	}

	do
	{
        std::string filename = epi::wstring_to_utf8(fdataw.cFileName);
	    if (filename == "." || filename == "..")
		{
		  // skip the funky "." and ".." dirs 
		}
        else if (fdataw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            std::string subdir = dir;
            subdir.push_back('/');
            subdir.append(filename);
            if (!FS_WalkDir(fsd, subdir))
                return false;
        }
		else
		{
            epi::dir_entry_c new_entry;
            new_entry.name = dir;
            new_entry.is_dir = false;
            new_entry.size = fdataw.nFileSizeLow;
            new_entry.name.push_back('/');
            new_entry.name.append(filename);
            fsd.push_back(new_entry);
		}
	}
	while (FindNextFileW(fhandle, &fdataw));

	FindClose(fhandle);
	FS_SetCurrDir(prev_dir);
	return true;
}
#else // POSIX API
static inline bool IsDirectorySeparator(const char c)
{
    return (c == '\\' || c == '/');
}
bool FS_IsAbsolute(std::string_view path)
{
	SYS_ASSERT(!path.empty());

    if (IsDirectorySeparator(path[0]))
		return true;
    else
        return false;
}
static const char *FlagsToANSIMode(int flags)
{
    // Must have some value in flags
    if (flags == 0)
        return nullptr;

    // Check for any invalid combinations
    if ((flags & kFileAccessWrite) && (flags & kFileAccessAppend))
        return nullptr;

    if (flags & kFileAccessRead)
    {
        if (flags & kFileAccessWrite)
            return "wb+";
        else if (flags & kFileAccessAppend)
            return "ab+";
        else
            return "rb";
    }
    else
    {
        if (flags & kFileAccessWrite)
            return "wb";
        else if (flags & kFileAccessAppend)
            return "ab";
        else
            return nullptr; // Invalid
    }
}
FILE *FS_OpenRawFile(std::string_view name, unsigned int flags)
{
    const char *mode = FlagsToANSIMode(flags);
    if (!mode) return nullptr;
    SYS_ASSERT(!name.empty());
    return fopen(std::string(name).c_str(), mode);
}
bool FS_Delete(std::string_view name)
{
    SYS_ASSERT(!name.empty());
    return remove(std::string(name).c_str()) == 0;
}
bool FS_IsDir(std::string_view dir)
{
    SYS_ASSERT(!dir.empty());
    struct stat dircheck;
    if (stat(std::string(dir).c_str(), &dircheck) == -1)
        return false;
    return S_ISDIR(dircheck.st_mode);
}
static std::string FS_GetCurrDir()
{
    std::string directory;
    const char *dir = getcwd(nullptr, 0);
    if (dir) directory = dir;
	return directory;
}
bool FS_SetCurrDir(std::string_view dir)
{
    SYS_ASSERT(!dir.empty());
    return chdir(std::string(dir).c_str()) == 0;
}
bool FS_MakeDir(std::string_view dir)
{
	SYS_ASSERT(!dir.empty());
	return (mkdir(std::string(dir).c_str(), 0664) == 0);
}
bool FS_Exists(std::string_view name)
{
    SYS_ASSERT(!name.empty());
    return access(std::string(name).c_str(), F_OK) == 0;
}
bool FS_Access(std::string_view name)
{
    // The codebase only seems to use this to test read access, so we
    // shouldn't need to pass any modes as a parameter
    SYS_ASSERT(!name.empty());
    return access(std::string(name).c_str(), R_OK) == 0;
}
bool FS_ReadDir(std::vector<dir_entry_c> &fsd, std::string &dir, const char *mask)
{
    if (dir.empty() || !FS_Exists(dir) || !mask)
        return false;

    std::string prev_dir = FS_GetCurrDir();

    std::string mask_ext = epi::FS_GetExtension(mask); // Allows us to retain "*.*" style syntax

    if (prev_dir.empty()) // Something goofed up, don't make it worse
        return false;

    if (!FS_SetCurrDir(dir))
        return false;

    DIR *handle = opendir(dir.c_str());
	if (!handle)
		return false;

    fsd.clear();

    for (;;)
	{
		struct dirent *fdata = readdir(handle);
		if (!fdata)
			break;

		if (strlen(fdata->d_name) == 0)
			continue;

        std::string filename = fdata->d_name;

		// skip the funky "." and ".." dirs 
		if (filename == "." || filename == "..")
			continue;

        // I though fnmatch should handle this, but ran into case sensitivity issues when 
        // using WSL for some reason - Dasho
        if (mask_ext != ".*" && epi::STR_CaseCmp(mask_ext, epi::FS_GetExtension(filename)) != 0)
            continue;

		struct stat finfo;

		if (stat(filename.c_str(), &finfo) != 0)
			continue;
		
		epi::dir_entry_c new_entry;
        new_entry.name = dir;
        new_entry.is_dir = S_ISDIR(finfo.st_mode) ? true : false;
        new_entry.size = finfo.st_size;
        new_entry.name.push_back('/');
        new_entry.name.append(filename);
        fsd.push_back(new_entry);
	}

	FS_SetCurrDir(prev_dir);
	closedir(handle);
	return true;
}
// Naive implementation; switch to nftw - Dasho
bool FS_WalkDir(std::vector<dir_entry_c> &fsd, std::string &dir)
{
    if (dir.empty() || !FS_Exists(dir))
        return false;

    std::string prev_dir = FS_GetCurrDir();

    if (prev_dir.empty()) // Something goofed up, don't make it worse
        return false;

    if (!FS_SetCurrDir(dir))
        return false;

    DIR *handle = opendir(dir.c_str());
	if (!handle)
		return false;

    for (;;)
	{
		struct dirent *fdata = readdir(handle);
		if (!fdata)
			break;

		if (strlen(fdata->d_name) == 0)
			continue;

        std::string filename = fdata->d_name;

		// skip the funky "." and ".." dirs 
		if (filename == "." || filename == "..")
			continue;

		struct stat finfo;

		if (stat(filename.c_str(), &finfo) != 0)
			continue;
		
        if (S_ISDIR(finfo.st_mode))
        {
            std::string subdir = dir;
            subdir.push_back('/');
            subdir.append(filename);
            if (!FS_WalkDir(fsd, subdir))
                return false;
        }
		else
		{
            epi::dir_entry_c new_entry;
            new_entry.name = dir;
            new_entry.is_dir = false;
            new_entry.size = finfo.st_size;
            new_entry.name.push_back('/');
            new_entry.name.append(filename);
            fsd.push_back(new_entry);
		}
	}

	FS_SetCurrDir(prev_dir);
	closedir(handle);
	return true;
}
#endif

// Universal Functions

std::string FS_GetStem(std::string_view path)
{
	SYS_ASSERT(!path.empty());
	// back up until a slash or the start
	for (int p = path.size() - 1; p > 1; p--)
    {
		if (IsDirectorySeparator(path[p-1]))
		{
            path.remove_prefix(p);
            break;
        }
    }
    // back up until a dot
	for (int p = path.size() - 2; p >= 0; p--)
    {
        const char ch = path[p];
		if (IsDirectorySeparator(ch))
			break;

		if (ch == '.')
		{
            // handle filenames that being with a dot
            // (un*x style hidden files)
            if (p == 0 || IsDirectorySeparator(path[p-1]))
				break;

			path.remove_suffix(path.size()-p);
            break;
		}
	}
    std::string filename(path);
    return filename;
}

std::string FS_GetFilename(std::string_view path)
{
	SYS_ASSERT(!path.empty());
	// back up until a slash or the start
	for (int p = path.size() - 1; p > 0; p--)
    {
		if (IsDirectorySeparator(path[p-1]))
		{
            path.remove_prefix(p);
            break;
        }
    }
    std::string filename(path);
    return filename;
}

// This should only be for EPK entry use; essentially it strips the parent
// path from the child path assuming the parent is actually in the child path
std::string FS_MakeRelative(std::string_view parent, std::string_view child)
{
    SYS_ASSERT(!parent.empty() && !child.empty() && child.size() > parent.size());
    size_t parent_check = child.find(parent);
    if (parent_check != std::string_view::npos)
    {
        child.remove_prefix(parent.size());
        if (IsDirectorySeparator(child[0]))
            child.remove_prefix(1);
    }
    std::string relpath(child);
    return relpath;
}

std::string FS_PathAppend(std::string_view parent, std::string_view child)
{
    SYS_ASSERT(!parent.empty() && !child.empty());

    if (IsDirectorySeparator(parent.back()))
        parent.remove_suffix(1);

    std::string new_path(parent);

    new_path.push_back('/');

    if (IsDirectorySeparator(child[0]))
        child.remove_prefix(1);

    new_path.append(child);

    return new_path;
}

std::string FS_GetDirectory(std::string_view path)
{
	SYS_ASSERT(!path.empty());
    std::string directory;
	// back up until a slash or the start
	for (int p = path.size() - 1; p >= 0; p--)
    {
		if (IsDirectorySeparator(path[p]))
        {
            directory = path.substr(0, p);
			break;
        }
    }

    return directory;  // nothing
}

std::string FS_GetExtension(std::string_view path)
{
	SYS_ASSERT(!path.empty());
    std::string extension;
    // back up until a dot
	for (int p = path.size() - 1; p >= 0; p--)
    {
        const char ch = path[p];
		if (IsDirectorySeparator(ch))
			break;

		if (ch == '.')
		{
            // handle filenames that being with a dot
            // (un*x style hidden files)
            if (p == 0 || IsDirectorySeparator(path[p-1]))
				break;

			extension = path.substr(p);
            break;
		}
	}
    return extension;  // can be empty
}

void FS_ReplaceExtension(std::string &path, std::string_view ext)
{
	SYS_ASSERT(!path.empty() && !ext.empty());
    int extpos = -1;
    // back up until a dot
	for (int p = path.size() - 1; p >= 0; p--)
    {
        char &ch = path[p];
		if (IsDirectorySeparator(ch))
			break;

		if (ch == '.')
		{
            // handle filenames that being with a dot
            // (un*x style hidden files)
            if (p == 0 || IsDirectorySeparator(path[p-1]))
				break;

			extpos = p;
            break;
		}
	}
    if (extpos == -1) // No extension found, add it
        path.append(ext);
    else
    {
        while (path.size() > extpos)
        {
            path.pop_back();
        }
        path.append(ext);
    }
}

file_c *FS_Open(std::string_view name, unsigned int flags)
{
    SYS_ASSERT(!name.empty());
    FILE *fp = FS_OpenRawFile(name, flags);
    if (!fp)
        return NULL;
    return new ansi_file_c(fp);
}

bool FS_OpenDir(const std::string &src)
{
    // A result of 0 is 'success', but that only means SDL was able to launch some kind of process
    // to attempt to handle the path. -1 is the only result that is guaranteed to be an 'error'
    if (SDL_OpenURL(STR_Format("file:///%s", src.c_str()).c_str()) == -1)
    {
        I_Warning("FS_OpenDir failed to open requested path %s\nError: %s\n", src.c_str(), SDL_GetError());
        return false;
    }
    return true;
}

bool FS_Copy(std::string_view src, std::string_view dest)
{
    SYS_ASSERT(!src.empty() && !dest.empty());

    if (!epi::FS_Access(src))
        return false;

    if (epi::FS_Exists(dest))
    {
        // overwrite dest if it exists
        if (!epi::FS_Delete(dest))
            return false;
    }

    file_c *srcfile = epi::FS_Open(src, kFileAccessRead | kFileAccessBinary);

    if (!srcfile) return false;

    file_c *destfile = epi::FS_Open(dest, kFileAccessWrite | kFileAccessBinary);

    if (!destfile)
    {
        delete srcfile;
        return false;
    }

    int srcsize = srcfile->GetLength();

    uint8_t *srcdata = srcfile->LoadIntoMemory();

    int copied = destfile->Write(srcdata, srcsize);

    delete[] srcdata;
    delete destfile;
    delete srcfile;

    if (copied != srcsize)
    {
        epi::FS_Delete(dest);
        return false;
    }
    else
        return true;
}

// Emscripten-specific
#ifdef EDGE_WEB
void FS_Sync(bool populate)
{
    EM_ASM_(
        {
            if (Module.edgePreSyncFS)
            {
                Module.edgePreSyncFS();
            }
            FS.syncfs(
                $0, function(err) {
                    if (Module.edgePostSyncFS)
                    {
                        Module.edgePostSyncFS();
                    }
                });
        },
        populate);
}
#else
void FS_Sync(bool populate)
{
    (void)populate;
}
#endif

} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
