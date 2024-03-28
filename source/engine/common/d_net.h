// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 18276462ef24770fc9c7c5f30e07df853870fdf5 $
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
//		Networking stuff
//
//-----------------------------------------------------------------------------

#pragma once

#define BACKUPTICS 12 // number of tics to remember

// Create any new ticcmds and broadcast to other players.
void NetUpdate();
