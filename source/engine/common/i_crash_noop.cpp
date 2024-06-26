// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 84a406fb35c118a7395e23ebe2d0dd37b956eabe $
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
//
// DESCRIPTION:
//   Noop Crash handling.
//
//-----------------------------------------------------------------------------

#if defined _WIN32 && defined _MSC_VER && !defined _DEBUG
#elif defined UNIX && defined HAVE_BACKTRACE
#else

#include "i_crash.h"
#include "mud_includes.h"

void I_SetCrashCallbacks()
{
    // Not implemented.
}

#endif
