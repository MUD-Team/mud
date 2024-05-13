// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 249bd22d60e133fc052b8d56b8293072d44e0766 $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Savegame I/O, archiving, persistence.
//
//-----------------------------------------------------------------------------

#pragma once

#include "farchive.h"

// Persistent storage/archiving.
// These are the load / save game routines.
// Also see farchive.(h|cpp)
void P_SerializePlayers(FArchive &arc);
void P_SerializeWorld(FArchive &arc);
void P_SerializeThinkers(FArchive &arc, bool);
void P_SerializeRNGState(FArchive &arc);
void P_SerializeSounds(FArchive &arc);
void P_SerializeACSDefereds(FArchive &arc);
void P_SerializePolyobjs(FArchive &arc);
