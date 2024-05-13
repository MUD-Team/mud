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
//  MUDProfiling header.
//
//-----------------------------------------------------------------------------

#pragma once

#ifdef __cplusplus

#ifdef MUD_PROFILING

#include <tracy/Tracy.hpp>

#define MUD_ZoneNamed(varname, active)                ZoneNamed(varname, active)
#define MUD_ZoneNamedN(varname, name, active)         ZoneNamedN(varname, name, active)
#define MUD_ZoneNamedC(varname, color, active)        ZoneNamedC(varname, color, active)
#define MUD_ZoneNamedNC(varname, name, color, active) ZoneNamedNC(varname, name, color, active)

#define MUD_ZoneScoped                ZoneScoped
#define MUD_ZoneScopedN(name)         ZoneScopedN(name)
#define MUD_ZoneScopedC(color)        ZoneScopedC(color)
#define MUD_ZoneScopedNC(name, color) ZoneScopedNC(name, color)

#define MUD_ZoneText(txt, size) ZoneText(txt, size)
#define MUD_ZoneName(txt, size) ZoneName(txt, size)

#define MUD_TracyPlot(name, val) TracyPlot(name, val)

#define MUD_FrameMark            FrameMark
#define MUD_FrameMarkNamed(name) FrameMarkNamed(name)
#define MUD_FrameMarkStart(name) FrameMarkStart(name)
#define MUD_FrameMarkEnd(name)   FrameMarkEnd(name)

#else

#define MUD_ZoneNamed(varname, active)
#define MUD_ZoneNamedN(varname, name, active)
#define MUD_ZoneNamedC(varname, color, active)
#define MUD_ZoneNamedNC(varname, name, color, active)

#define MUD_ZoneScoped
#define MUD_ZoneScopedN(name)
#define MUD_ZoneScopedC(color)
#define MUD_ZoneScopedNC(name, color)

#define MUD_ZoneText(txt, size)
#define MUD_ZoneName(txt, size)

#define MUD_TracyPlot(name, val)

#define MUD_FrameMark
#define MUD_FrameMarkNamed(name)
#define MUD_FrameMarkStart(name)
#define MUD_FrameMarkEnd(name)

#endif

#else
#include <tracy/TracyC.h>
#endif
