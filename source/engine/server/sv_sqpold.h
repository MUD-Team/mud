// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: a56dbccecf1d6cda7c62923d5a86549fa01fa1de $
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//  Old version of the server query protocol, kept for clients and older
//  launchers
//
//-----------------------------------------------------------------------------

#pragma once

void SV_SendServerInfo();
bool SV_IsValidToken(uint32_t token);
