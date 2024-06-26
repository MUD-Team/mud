// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: ebda9fa19025ff1c0a8c61352f69f5bb145141b0 $
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
//		Main loop menu stuff.
//		Random number LUT.
//		Default Config File.
//		PCX Screenshots.
//
//-----------------------------------------------------------------------------

#include "m_bbox.h"

#include "mud_includes.h"

IMPLEMENT_CLASS(DBoundingBox, DObject)

DBoundingBox::DBoundingBox()
{
    ClearBox();
}

void DBoundingBox::ClearBox()
{
    m_Box[BOXTOP] = m_Box[BOXRIGHT] = INT32_MIN;
    m_Box[BOXBOTTOM] = m_Box[BOXLEFT] = INT32_MAX;
}

void DBoundingBox::AddToBox(fixed_t x, fixed_t y)
{
    if (x < m_Box[BOXLEFT])
        m_Box[BOXLEFT] = x;
    else if (x > m_Box[BOXRIGHT])
        m_Box[BOXRIGHT] = x;

    if (y < m_Box[BOXBOTTOM])
        m_Box[BOXBOTTOM] = y;
    else if (y > m_Box[BOXTOP])
        m_Box[BOXTOP] = y;
}

VERSION_CONTROL(m_bbox_cpp, "$Id: ebda9fa19025ff1c0a8c61352f69f5bb145141b0 $")
