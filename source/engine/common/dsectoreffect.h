// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: be67bdfc692b11152054ecf4d7cfee31420e235a $
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
//	Separated Sector effects (?)
//
//-----------------------------------------------------------------------------

#pragma once

#include "dobject.h"
#include "dthinker.h"
#include "r_defs.h"

typedef enum
{
    SEC_INVALID,
    SEC_FLOOR,
    SEC_PLAT,
    SEC_CEILING,
    SEC_DOOR,
    SEC_ELEVATOR,
    SEC_PILLAR,
    //	SEC_WAGGLE,	// We don't send sector updates for these
} movertype_t;

class DSectorEffect : public DThinker
{
    DECLARE_SERIAL(DSectorEffect, DThinker)
  public:
    DSectorEffect(sector_t *sector);
    ~DSectorEffect();
    virtual DSectorEffect *Clone(sector_t *sector) const;
    virtual void           Destroy();
    sector_t              *GetSector() const
    {
        return m_Sector;
    }

  protected:
    DSectorEffect();
    sector_t *m_Sector;
};

class DMover : public DSectorEffect
{
    DECLARE_SERIAL(DMover, DSectorEffect);

  public:
    DMover(sector_t *sector);

  protected:
    enum EResult
    {
        ok,
        crushed,
        pastdest
    };

  private:
    EResult MovePlane(fixed_t speed, fixed_t dest, int32_t crush, int32_t floorOrCeiling, int32_t direction,
                      bool hexencrush);

  protected:
    DMover();
    inline EResult MoveFloor(fixed_t speed, fixed_t dest, int32_t crush, int32_t direction, bool hexencrush)
    {
        return MovePlane(speed, dest, crush, 0, direction, hexencrush);
    }
    inline EResult MoveFloor(fixed_t speed, fixed_t dest, int32_t direction)
    {
        return MovePlane(speed, dest, NO_CRUSH, 0, direction, false);
    }
    inline EResult MoveCeiling(fixed_t speed, fixed_t dest, int32_t crush, int32_t direction, bool hexencrush)
    {
        return MovePlane(speed, dest, crush, 1, direction, hexencrush);
    }
    inline EResult MoveCeiling(fixed_t speed, fixed_t dest, int32_t direction)
    {
        return MovePlane(speed, dest, NO_CRUSH, 1, direction, false);
    }
};

class DMovingFloor : public DMover
{
    DECLARE_SERIAL(DMovingFloor, DMover);

  public:
    DMovingFloor(sector_t *sector);

  protected:
    DMovingFloor();
};

class DMovingCeiling : public DMover
{
    DECLARE_SERIAL(DMovingCeiling, DMover);

  public:
    DMovingCeiling(sector_t *sector);

  protected:
    DMovingCeiling();
};
