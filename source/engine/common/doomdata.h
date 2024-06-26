// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 2bbe997eb70a777aede3e193d11be3e6582e120d $
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
//  all external data is defined here
//  most of the data is loaded into different structures at run time
//  some internal structures shared by many modules are here
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomtype.h"
#include "farchive.h"

//
// Map level types.
// The following data structures define the persistent format
// used in the lumps of the WAD files.
//

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum
{
    ML_LABEL,    // A separator, name, ExMx or MAPxx
    ML_THINGS,   // Monsters, items
    ML_LINEDEFS, // LineDefs, from editing
    ML_SIDEDEFS, // SideDefs, from editing
    ML_VERTEXES, // Vertices, edited and BSP splits generated
    ML_SEGS,     // LineSegs, from LineDefs split by BSP
    ML_SSECTORS, // SubSectors, list of LineSegs
    ML_NODES,    // BSP nodes
    ML_SECTORS,  // Sectors, from editing
    ML_REJECT,   // LUT, sector-sector visibility
    ML_BLOCKMAP, // LUT, motion clipping, walls/grid element
    ML_BEHAVIOR  // [RH] Hexen-style scripts. If present, THINGS
                 //		and LINEDEFS are also Hexen-style.
};

// A single Vertex.
typedef struct
{
    int16_t x;
    int16_t y;
} mapvertex_t;

// A SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
typedef struct
{
    int16_t textureoffset;
    int16_t rowoffset;
    char    toptexture[8];
    char    bottomtexture[8];
    char    midtexture[8];
    int16_t sector; // Front sector, towards viewer.
} mapsidedef_t;

// A LineDef, as used for editing, and as input to the BSP builder.
typedef struct
{
    uint16_t v1;
    uint16_t v2;
    int16_t  flags;
    int16_t  special;
    int16_t  tag;
    // sidenum[1] will be -1 if one sided
    int16_t sidenum[2];
} maplinedef_t;

// A ZDoom style LineDef, from the Doom Wiki.
typedef struct
{
    uint16_t v1;
    uint16_t v2;
    int16_t  flags;
    uint8_t  special;
    uint8_t  args[5];
    // sidenum[1] will be -1 if one sided
    int16_t sidenum[2];
} maplinedef2_t;

//
// LineDef attributes.
//

#define ML_BLOCKING      0x0001 // solid, is an obstacle
#define ML_BLOCKMONSTERS 0x0002 // blocks monsters only
#define ML_TWOSIDED      0x0004 // backside will not be present at all if not two sided

// If a texture is pegged, the texture will have
// the end exposed to air held constant at the
// top or bottom of the texture (stairs or pulled
// down things) and will move with a height change
// of one of the neighbor sectors.
// Unpegged textures always have the first row of
// the texture at the top pixel of the line for both
// top and bottom textures (use next to windows).

#define ML_DONTPEGTOP    0x0008 // upper texture unpegged
#define ML_DONTPEGBOTTOM 0x0010 // lower texture unpegged
#define ML_SECRET        0x0020 // don't map as two sided: IT'S A SECRET!
#define ML_SOUNDBLOCK    0x0040 // don't let sound cross two of these
#define ML_DONTDRAW      0x0080 // don't draw on the automap
#define ML_MAPPED        0x0100 // set if already drawn in automap

// jff 3/21/98 Set if line absorbs use by player
// allow multiple push/switch triggers to be used on one push
#define ML_PASSUSE 0x0200

// Reserved by EE
// SoM 9/02/02: 3D Middletexture flag!
#define ML_3DMIDTEX 0x0400

// haleyjd 05/02/06: Although it was believed until now that a reserved line
// flag was unnecessary, a problem with Ultimate DOOM E2M7 has disproven this
// theory. It has roughly 1000 linedefs with 0xFE00 masked into the flags, so
// making the next line flag reserved and using it to toggle off ALL extended
// flags will preserve compatibility for such maps. I have been told this map
// is one of the first ever created, so it may have something to do with that.
#define ML_RESERVED 0x0800

// [Blair] MBF21 Line flags
#define ML_BLOCKLANDMONSTERS 0x1000

#define ML_BLOCKPLAYERS 0x2000

// Hexen/ZDoom stuff

#define ML_MONSTERSCANACTIVATE 0x4000   // zdoom
#define ML_BLOCKEVERYTHING     0x8000   // zdoom

#define ML_REPEATSPECIAL 0x00010000     // special is repeatable

#define ML_SPAC_CROSS        0x00020000 // hexen activation
#define ML_SPAC_USE          0x00040000 // hexen activation
#define ML_SPAC_MCROSS       0x00080000 // hexen activation
#define ML_SPAC_IMPACT       0x00100000 // hexen activation
#define ML_SPAC_PUSH         0x00200000 // hexen activation
#define ML_SPAC_PCROSS       0x00400000 // hexen activation
#define ML_SPAC_USETHROUGH   0x00800000 // SPAC_USE, but passes it through
#define ML_SPAC_CROSSTHROUGH 0x01600000 // SPAC_CROSS, but passes it through

#define ML_SPAC_SHIFT 10
#define ML_SPAC_MASK                                                                                                   \
    (ML_SPAC_CROSS | ML_SPAC_USE | ML_SPAC_MCROSS | ML_SPAC_IMPACT | ML_SPAC_PUSH | ML_SPAC_PCROSS |                   \
     ML_SPAC_USETHROUGH | ML_SPAC_CROSSTHROUGH)
#define GET_SPAC(flags) ((flags & ML_SPAC_MASK) >> ML_SPAC_SHIFT)

// hexen
#define HML_REPEATSPECIAL 0x0200 // special is repeatable
#define HML_SPAC_SHIFT    10
#define HML_SPAC_MASK     0x1c00
#define GET_HSPAC(flags)  ((flags & HML_SPAC_MASK) >> HML_SPAC_SHIFT)

// zdoom
#define ZML_MONSTERSCANACTIVATE 0x2000 // Monsters and players can activate
#define ZML_BLOCKPLAYERS        0x4000 // Blocks players
#define ZML_BLOCKEVERYTHING     0x8000 // Blocks everything

// Sector definition, from editing
typedef struct
{
    int16_t floorheight;
    int16_t ceilingheight;
    char    floorpic[8];
    char    ceilingpic[8];
    int16_t lightlevel;
    int16_t special;
    int16_t tag;
} mapsector_t;

// SubSector, as generated by BSP
typedef struct
{
    int16_t numsegs;
    int16_t firstseg; // index of first one, segs are stored sequentially
} mapsubsector_t;

// LineSeg, generated by splitting LineDefs
// using partition lines selected by BSP builder.
typedef struct
{
    int16_t v1;
    int16_t v2;
    int16_t angle;
    int16_t linedef;
    int16_t side;
    int16_t offset;
} mapseg_t;

// BSP node structure.

typedef struct
{
    // Partition line from (x,y) to x+dx,y+dy)
    int16_t x;
    int16_t y;
    int16_t dx;
    int16_t dy;

    // Bounding box for each child,
    // clip against view frustum.
    int16_t bbox[2][4];

    // If top bit is set, it's a subsector,
    // else it's a node of another subtree.
    uint16_t children[2];

} mapnode_t;

// Thing definition, position, orientation and type,
// plus skill/visibility flags and attributes.
// Thing for Doom.
typedef struct
{
    int16_t x;
    int16_t y;
    int16_t angle;
    int16_t type;
    int16_t options;
} mapthing_t;

// [RH] Hexen-compatible MapThing.
typedef struct MapThing
{
    uint16_t thingid;
    int16_t  x;
    int16_t  y;
    int16_t  z;
    int16_t  angle;
    int16_t  type;
    int16_t  flags;
    uint8_t  special;
    uint8_t  args[5];

    void Serialize(FArchive &);
} mapthing2_t;

#define NO_INDEX ((uint16_t) - 1)

// [RH] MapThing flags.

#define MTF_EASY   0x0001      // Thing will appear on easy skill setting
#define MTF_MEDIUM 0x0002      // Thing will appear on medium skill setting
#define MTF_HARD   0x0004      // Thing will appear on hard skill setting
#define MTF_AMBUSH 0x0008      // Thing is deaf

#define MTF_DORMANT     0x0010 // Thing is dormant (use Thing_Activate)
#define MTF_SINGLE      0x0100 // Thing appears in single-player games
#define MTF_COOPERATIVE 0x0200 // Thing appears in cooperative games
#define MTF_DEATHMATCH  0x0400 // Thing appears in deathmatch games

// Custom MapThing Flags
#define MTF_FILTER_COOPWPN                                                                                             \
    0x0800 // Weapon thing is filtered with g_thingfilter 1.
           // (Hate this method but it works...)

// BOOM and DOOM compatible versions of some of the above

#define BTF_NOTSINGLE      0x0010 // (TF_COOPERATIVE|TF_DEATHMATCH)
#define BTF_NOTDEATHMATCH  0x0020 // (TF_SINGLE|TF_COOPERATIVE)
#define BTF_NOTCOOPERATIVE 0x0040 // (TF_SINGLE|TF_DEATHMATCH)

#define NO_CRUSH   -1
#define DOOM_CRUSH 10

//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//
typedef struct
{
    int16_t originx;
    int16_t originy;
    int16_t patch;
    int16_t stepdir;
    int16_t colormap;
} mappatch_t;

//
// Texture definition.
// A DOOM wall texture is a list of patches
// which are to be combined in a predefined order.
//
typedef struct
{
    char       name[8];
    uint16_t   masked;             // [RH] Unused
    uint8_t    scalex;             // [RH] Scaling (8 is normal)
    uint8_t    scaley;             // [RH] Same as above
    int16_t    width;
    int16_t    height;
    uint8_t    columndirectory[4]; // OBSOLETE
    int16_t    patchcount;
    mappatch_t patches[1];
} maptexture_t;
