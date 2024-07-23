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

#include "epi_filesystem.h"

#include "epi.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"

static constexpr char null_padding = '\0';

namespace epi
{

File::File(PHYSFS_File *filep) : fp_(filep)
{
}

File::~File()
{
    if (fp_)
    {
        PHYSFS_close(fp_);
        fp_ = nullptr;
    }
}

uint64_t File::GetLength()
{
    EPI_ASSERT(fp_);

    return PHYSFS_fileLength(fp_);
}

int64_t File::GetPosition()
{
    EPI_ASSERT(fp_);

    return PHYSFS_tell(fp_);
}

int64_t File::Read(void *dest, uint64_t size)
{
    EPI_ASSERT(fp_);
    EPI_ASSERT(dest);

    return PHYSFS_readBytes(fp_, dest, size);
}

int64_t File::Write(const void *src, uint64_t size)
{
    EPI_ASSERT(fp_);
    EPI_ASSERT(src);

    return PHYSFS_writeBytes(fp_, src, size);
}

int64_t File::WriteString(std::string_view str)
{
    EPI_ASSERT(fp_);
    EPI_ASSERT(!str.empty());
    int64_t written = PHYSFS_writeBytes(fp_, str.data(), str.size());
    written += PHYSFS_writeBytes(fp_, &null_padding, 1);
    return written;
}

bool File::Seek(uint64_t offset)
{
    return (PHYSFS_seek(fp_, offset) != 0);
}

std::string File::ReadAsString()
{
    std::string textstring;
    Seek(0);
    uint8_t *buffer = LoadIntoMemory();
    if (buffer)
    {
        textstring.assign((char *)buffer, GetLength());
        delete[] buffer;       
    }
    return textstring;
}

uint8_t *File::LoadIntoMemory()
{
    int cur_pos     = GetPosition();
    int actual_size = GetLength();

    actual_size -= cur_pos;

    if (actual_size < 0)
    {
        LogWarning("File::LoadIntoMemory : position > length.\n");
        actual_size = 0;
    }

    uint8_t *buffer     = new uint8_t[actual_size + 1];
    buffer[actual_size] = 0;

    if ((int)Read(buffer, actual_size) != actual_size)
    {
        delete[] buffer;
        return nullptr;
    }

    return buffer; // success!
}

#ifdef _WIN32
static inline bool IsDirectorySeparator(const char c)
{
    return (c == '\\' || c == '/' || c == ':'); // Kester added ':'
}
#else
static inline bool IsDirectorySeparator(const char c)
{
    return (c == '\\' || c == '/');
}
#endif

// Universal Functions

std::string GetStem(std::string_view path)
{
    EPI_ASSERT(!path.empty());
    // back up until a slash or the start
    for (int p = path.size() - 1; p >= 1; p--)
    {
        if (IsDirectorySeparator(path[p - 1]))
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
            // (unix style hidden files)
            if (p == 0 || IsDirectorySeparator(path[p - 1]))
                break;

            path.remove_suffix(path.size() - p);
            break;
        }
    }
    std::string filename(path);
    return filename;
}

std::string GetFilename(std::string_view path)
{
    EPI_ASSERT(!path.empty());
    // back up until a slash or the start
    for (int p = path.size() - 1; p >= 1; p--)
    {
        if (IsDirectorySeparator(path[p - 1]))
        {
            path.remove_prefix(p);
            break;
        }
    }
    std::string filename(path);
    return filename;
}

std::string SanitizePath(std::string_view path)
{
    std::string sani_path;
    for (const char ch : path)
    {
        if (ch == '\\')
            sani_path.push_back('/');
        else
            sani_path.push_back(ch);
    }
    return sani_path;
}

std::string PathAppend(std::string_view parent, std::string_view child)
{
    EPI_ASSERT(!parent.empty() && !child.empty());

    if (IsDirectorySeparator(parent.back()))
        parent.remove_suffix(1);

    std::string new_path(parent);

    new_path.push_back('/');

    if (IsDirectorySeparator(child[0]))
        child.remove_prefix(1);

    new_path.append(child);

    return new_path;
}

std::string GetDirectory(std::string_view path)
{
    EPI_ASSERT(!path.empty());
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

    return directory; // nothing
}

std::string GetExtension(std::string_view path)
{
    EPI_ASSERT(!path.empty());
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
            // (unix style hidden files)
            if (p == 0 || IsDirectorySeparator(path[p - 1]))
                break;

            extension = path.substr(p);
            break;
        }
    }
    return extension; // can be empty
}

File *FileOpen(const std::string &name, unsigned int flags)
{
    EPI_ASSERT(!name.empty());
    PHYSFS_File *fp = nullptr;
    switch (flags) // should only be one
    {
        case kFileAccessRead:
            fp = PHYSFS_openRead(name.c_str());
            break;
        case kFileAccessWrite:
            fp = PHYSFS_openWrite(name.c_str());
            break;
        case kFileAccessAppend:
            fp = PHYSFS_openAppend(name.c_str());
            break;
        default:
            FatalError("FileOpen called with invalid mode!\n");
            break;
    }
    if (!fp)
        return nullptr;
    return new File(fp);
}

bool FileDelete(const std::string &name)
{
    EPI_ASSERT(!name.empty());
    return (PHYSFS_delete(name.c_str()) != 0);
}

bool FileExists(const std::string &name)
{
    EPI_ASSERT(!name.empty());
    return (PHYSFS_exists(name.c_str()) != 0);
}

bool MakeDirectory(const std::string &name)
{
    EPI_ASSERT(!name.empty());
    return (PHYSFS_mkdir(name.c_str()) != 0);
}

} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
