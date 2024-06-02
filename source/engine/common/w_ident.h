// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 6dbc0dc013e5f232dccfc9450bfa301c5e8c3f71 $
//
// Copyright (C) 2006-2020 by The Odamex Team.
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
//
// Resource file identification
//
//-----------------------------------------------------------------------------

#pragma once

#include "m_ostring.h"
#include "m_resfile.h"

struct fileIdentifier_t
{
    OString     mIdName;
    std::string mNiceName;
    OString     mFilename;
    bool        mIsIWAD;
};

void                 W_ConfigureGameInfo(const OResFile &iwad);
bool                 W_IsKnownIWAD(const OWantFile &file);
bool                 W_IsIWAD(const OResFile &file);
std::vector<OString> W_GetIWADFilenames();
