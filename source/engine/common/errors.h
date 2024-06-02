// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 950c890fc4f20f4eca6bc7bb1b1b93a2c018a6c0 $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Error Handling
//
//-----------------------------------------------------------------------------

#pragma once

#include <Poco/Exception.h>

#define DOOM_EXCEPTION_API

POCO_DECLARE_EXCEPTION(DOOM_EXCEPTION_API, CDoomError, Poco::Exception)
