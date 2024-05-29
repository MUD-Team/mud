// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 07e2be845a7edbf17ed817c2d972e3f15b48b97d $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
// Copyright (C) 2024 by The MUD Team.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//	Common Windows includes and defines
//
//-----------------------------------------------------------------------------

#pragma once

#ifdef _WIN32

// DrawText macros in winuser.h interfere with v_video.h
#ifndef NODRAWTEXT
#define NODRAWTEXT
#endif // NODRAWTEXT

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX;

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

// avoid a conflict with the winuser.h macro DrawText
#ifdef DrawText
#undef DrawText
#endif

// Same with PlaySound
#ifdef PlaySound
#undef PlaySound
#endif

#if (defined _MSC_VER)
#define strncasecmp _strnicmp
#endif

#endif // WIN32
