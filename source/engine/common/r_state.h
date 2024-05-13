// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: e463ead1c94fe0ba096aa0a1f05457dea4683779 $
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
//	Refresh/render internal state variables (global).
//
//-----------------------------------------------------------------------------

#pragma once

// Need data structure definitions.
#include "d_player.h"
#include "r_data.h"

#define WALLFRACBITS 4
#define WALLFRACUNIT (1 << WALLFRACBITS)

//
// Refresh internal data structures,
//	for rendering.
//

extern "C" int32_t viewwidth;
extern "C" int32_t viewheight;

//
// Lookup tables for map data.
//
extern bool g_ValidLevel;

extern int32_t          numsprites;
extern spritedef_t *sprites;

extern int32_t       numvertexes;
extern vertex_t *vertexes;

extern int32_t    numsegs;
extern seg_t *segs;

extern int32_t       numsectors;
extern sector_t *sectors;

extern int32_t          numsubsectors;
extern subsector_t *subsectors;

extern int32_t     numnodes;
extern node_t *nodes;

extern int32_t     numlines;
extern line_t *lines;

extern int32_t     numsides;
extern side_t *sides;

extern std::vector<int32_t> originalLightLevels;

inline FArchive &operator<<(FArchive &arc, sector_t *sec)
{
    if (sec)
        return arc << (uint16_t)(sec - sectors);
    else
        return arc << (uint16_t)0xffff;
}
inline FArchive &operator>>(FArchive &arc, sector_t *&sec)
{
    uint16_t ofs;
    arc >> ofs;
    if (ofs == 0xffff)
        sec = NULL;
    else
        sec = sectors + ofs;
    return arc;
}

inline FArchive &operator<<(FArchive &arc, line_t *line)
{
    if (line)
        return arc << (uint16_t)(line - lines);
    else
        return arc << (uint16_t)0xffff;
}
inline FArchive &operator>>(FArchive &arc, line_t *&line)
{
    uint16_t ofs;
    arc >> ofs;
    if (ofs == 0xffff)
        line = NULL;
    else
        line = lines + ofs;
    return arc;
}

struct LocalView
{
    angle_t angle;
    bool    setangle;
    bool    skipangle;
    int32_t     pitch;
    bool    setpitch;
    bool    skippitch;
};

//
// POV data.
//
extern fixed_t viewx;
extern fixed_t viewy;
extern fixed_t viewz;

extern angle_t   viewangle;
extern LocalView localview;
extern AActor   *camera; // [RH] camera instead of viewplayer

extern angle_t clipangle;

// extern fixed_t		finetangent[FINEANGLES/2];

extern visplane_t *floorplane;
extern visplane_t *ceilingplane;
extern visplane_t *skyplane;

// [AM] 4:3 Field of View
extern int32_t FieldOfView;
// [AM] Corrected (for widescreen) Field of View
extern int32_t CorrectFieldOfView;
