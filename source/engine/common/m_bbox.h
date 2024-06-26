// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 7e62889f623ea4fe61d637d8a16db52be9b6173b $
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

#pragma once

#include "dobject.h"
#include "m_fixed.h"

// Bounding box coordinate storage.
enum
{
    BOXTOP,
    BOXBOTTOM,
    BOXLEFT,
    BOXRIGHT
}; // bbox coordinates

class DBoundingBox : public DObject
{
    DECLARE_CLASS(DBoundingBox, DObject)
  public:
    DBoundingBox();

    void ClearBox();
    void AddToBox(fixed_t x, fixed_t y);

    inline fixed_t Top()
    {
        return m_Box[BOXTOP];
    }
    inline fixed_t Bottom()
    {
        return m_Box[BOXBOTTOM];
    }
    inline fixed_t Left()
    {
        return m_Box[BOXLEFT];
    }
    inline fixed_t Right()
    {
        return m_Box[BOXRIGHT];
    }

  protected:
    fixed_t m_Box[4];
};
