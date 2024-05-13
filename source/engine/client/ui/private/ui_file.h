// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
//
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
//  UIFile header.
//
//-----------------------------------------------------------------------------

#pragma once

#include <RmlUi/Core/FileInterface.h>

class UIFileInterface : public Rml::FileInterface
{
  public:
    UIFileInterface();
    virtual ~UIFileInterface();

    /// Opens a file.
    Rml::FileHandle Open(const Rml::String &path) override;

    /// Closes a previously opened file.
    void Close(Rml::FileHandle file) override;

    /// Reads data from a previously opened file.
    size_t Read(void *buffer, size_t size, Rml::FileHandle file) override;

    /// Seeks to a point in a previously opened file.
    bool Seek(Rml::FileHandle file, long offset, int32_t origin) override;

    /// Returns the current position of the file pointer.
    size_t Tell(Rml::FileHandle file) override;

  private:
    Rml::String root;
};
