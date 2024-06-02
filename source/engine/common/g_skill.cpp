// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
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
//   Skill data for defining new skills.
//
//-----------------------------------------------------------------------------

#include "g_skill.h"

#include "mud_includes.h"

SkillInfo SkillInfos[MAX_SKILLS];
uint8_t   skillnum         = 0;
uint8_t   defaultskillmenu = 2;

const SkillInfo &G_GetCurrentSkill()
{
    return SkillInfos[sv_skill.asInt() - 1];
}