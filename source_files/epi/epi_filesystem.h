//----------------------------------------------------------------------------
//  EDGE Filesystem API
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

#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "../../libraries/physfs/physfs.h" 
namespace epi
{

// Access Types
enum Access
{
    kFileAccessRead   = 0x1,
    kFileAccessWrite  = 0x2,
    kFileAccessAppend = 0x4
};

class File
{
  private:
    PHYSFS_File *fp_;

  public:
    File(PHYSFS_File *filep);
    ~File();

    uint64_t GetLength();
    int64_t GetPosition();

    int64_t Read(void *dest, uint64_t size);
    int64_t Write(const void *src, uint64_t size);

    std::string ReadAsString();
    int64_t WriteString(std::string_view str);

    bool Seek(uint64_t offset);

    // load the file into memory, reading from the current
    // position.  An extra NUL byte is appended
    // to the result buffer.  Returns nullptr on failure.
    // The returned buffer must be freed with delete[].
    uint8_t *LoadIntoMemory();
};

// Path and Filename Functions
std::string GetFilename(std::string_view path);
std::string GetStem(std::string_view path);
std::string GetDirectory(std::string_view path);
std::string GetExtension(std::string_view path);
std::string PathAppend(std::string_view parent, std::string_view child);
std::string SanitizePath(std::string_view path);

// Directory Functions
bool MakeDirectory(const std::string &name);

// File Functions
File *FileOpen(const std::string &name, unsigned int flags);
bool  FileDelete(const std::string &name);
bool  FileExists(const std::string &name);

} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
