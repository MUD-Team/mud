// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: cb76b8e6a9fff8e6ec132bbd8132ae61f1244a67 $
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
//		Do all the WAD I/O, get map description,
//		set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------

#include "p_setup.h"

#include <math.h>
#include <stdlib.h>

#include <set>

#include "c_console.h"
#include "g_game.h"
#include "g_gametype.h"
#include "i_system.h"
#include "m_alloc.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_vectors.h"
#include "md5.h"
#include "mud_includes.h"
#include "p_acs.h"
#include "p_lnspec.h"
#include "p_local.h"
#include "p_mapformat.h"
#include "p_mobj.h"
#include "Poco/ByteOrder.h"
#include "r_common.h"
#include "r_sprites.h"
#include "s_sound.h"
#include "v_palette.h"
#include "w_wad.h"
#include "z_zone.h"

void SV_PreservePlayer(player_t &player);
void P_SpawnMapThing(mapthing2_t *mthing, int32_t position);
void P_SpawnAvatars();
void P_TranslateTeleportThings();

const uint32_t P_TranslateCompatibleLineFlags(const uint32_t flags, const bool reserved);
const uint32_t P_TranslateZDoomLineFlags(const uint32_t flags);
void           P_SpawnCompatibleSectorSpecial(sector_t *sector);

static void P_SetupLevelFloorPlane(sector_t *sector);
static void P_SetupLevelCeilingPlane(sector_t *sector);
static void P_SetupSlopes();
void        P_InvertPlane(plane_t *plane);
void        P_SetupWorldState();
int32_t     P_TranslateSectorSpecial(int32_t special);

extern dyncolormap_t NormalLight;
extern AActor       *shootthing;

EXTERN_CVAR(g_thingfilter)

bool g_ValidLevel = false;

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int32_t   numvertexes;
vertex_t *vertexes;

int32_t numsegs;
seg_t  *segs;

int32_t   numsectors;
sector_t *sectors;

int32_t      numsubsectors;
subsector_t *subsectors;

int32_t numnodes;
node_t *nodes;

int32_t numlines;
line_t *lines;

int32_t numsides;
side_t *sides;

std::vector<int32_t> originalLightLevels; // Needed for map resets

// [RH] Set true if the map contains a BEHAVIOR lump
bool HasBehavior = false;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int32_t bmapwidth;
int32_t bmapheight;    // size in mapblocks

int32_t *blockmap;     // int32_t for larger maps ([RH] Made int32_t because BOOM does)
int32_t *blockmaplump; // offsets in blockmap are from here

fixed_t bmaporgx;      // origin of block map
fixed_t bmaporgy;

AActor **blocklinks;   // for thing chains

// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//	LineOf Sight calculation.
// Without special effect, this could be
//	used as a PVS lookup as well.
//
uint8_t *rejectmatrix;
bool     rejectempty;

// Maintain single and multi player starting spots.
std::vector<mapthing2_t> DeathMatchStarts;
std::vector<mapthing2_t> playerstarts;
std::vector<mapthing2_t> voodoostarts;

//
// P_LoadVertexes
//
void P_LoadVertexes(int32_t lump)
{
    uint8_t *data;
    int32_t  i;

    // Determine number of vertices:
    //	total lump length / vertex record length.
    numvertexes = W_LumpLength(lump) / sizeof(mapvertex_t);

    // Allocate zone memory for buffer.
    vertexes = (vertex_t *)Z_Malloc(numvertexes * sizeof(vertex_t), PU_LEVEL, 0);

    // Load data into cache.
    data = (uint8_t *)W_CacheLumpNum(lump, PU_STATIC);

    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    for (i = 0; i < numvertexes; i++)
    {
        vertexes[i].x = Poco::ByteOrder::fromLittleEndian(((mapvertex_t *)data)[i].x) << FRACBITS;
        vertexes[i].y = Poco::ByteOrder::fromLittleEndian(((mapvertex_t *)data)[i].y) << FRACBITS;
    }

    // Free buffer memory.
    Z_Free(data);
}

//
// P_LoadSegs
//
// killough 5/3/98: reformatted, cleaned up

void P_LoadSegs(int32_t lump)
{
    if (!W_LumpLength(lump))
    {
        I_Error("P_LoadSegs: SEGS lump is empty - levels without nodes are not supported.");
    }

    int32_t  i;
    uint8_t *data;

    numsegs = W_LumpLength(lump) / sizeof(mapseg_t);
    segs    = (seg_t *)Z_Malloc(numsegs * sizeof(seg_t), PU_LEVEL, 0);
    memset(segs, 0, numsegs * sizeof(seg_t));
    data = (uint8_t *)W_CacheLumpNum(lump, PU_STATIC);

    for (i = 0; i < numsegs; i++)
    {
        seg_t    *li = segs + i;
        mapseg_t *ml = (mapseg_t *)data + i;

        int32_t side, linedef;
        line_t *ldef;

        uint16_t v = Poco::ByteOrder::fromLittleEndian(ml->v1);

        if (v >= numvertexes)
            I_Error("P_LoadSegs: invalid vertex %d", v);
        else
            li->v1 = &vertexes[v];

        v = Poco::ByteOrder::fromLittleEndian(ml->v2);

        if (v >= numvertexes)
            I_Error("P_LoadSegs: invalid vertex %d", v);
        else
            li->v2 = &vertexes[v];

        li->angle = (Poco::ByteOrder::fromLittleEndian(ml->angle)) << 16;

        li->offset = (Poco::ByteOrder::fromLittleEndian(ml->offset)) << 16;
        linedef    = Poco::ByteOrder::fromLittleEndian(ml->linedef);

        if (linedef < 0 || linedef >= numlines)
            I_Error("P_LoadSegs: invalid linedef %d", linedef);

        ldef        = &lines[linedef];
        li->linedef = ldef;

        side = Poco::ByteOrder::fromLittleEndian(ml->side);

        if (side != 0 && side != 1)
            side = 1; // assume invalid value means back

        li->sidedef     = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;

        // killough 5/3/98: ignore 2s flag if second sidedef missing:
        if (ldef->flags & ML_TWOSIDED && ldef->sidenum[side ^ 1] != R_NOSIDE)
            li->backsector = sides[ldef->sidenum[side ^ 1]].sector;
        else
        {
            li->backsector = 0;
            ldef->flags &= ~ML_TWOSIDED;
        }

        // recalculate seg offsets. values in wads are untrustworthy.
        vertex_t *from = (side == 0) ? ldef->v1  // right side: offset is from start of linedef
                                     : ldef->v2; // left side: offset is from end of linedef
        vertex_t *to   = li->v1;                 // end point is start of seg, in both cases

        float dx   = FIXED2FLOAT(to->x - from->x);
        float dy   = FIXED2FLOAT(to->y - from->y);
        li->offset = FLOAT2FIXED(sqrt(dx * dx + dy * dy));

        dx         = FIXED2FLOAT(li->v2->x - li->v1->x);
        dy         = FIXED2FLOAT(li->v2->y - li->v1->y);
        li->length = FLOAT2FIXED(sqrt(dx * dx + dy * dy));
    }

    Z_Free(data);
}

//
// P_LoadSubsectors
//
void P_LoadSubsectors(int32_t lump)
{
    if (!W_LumpLength(lump))
    {
        I_Error("P_LoadSubsectors: SSECTORS lump is empty - levels without nodes are not supported.");
    }

    uint8_t *data;
    int32_t  i;

    numsubsectors = W_LumpLength(lump) / sizeof(mapsubsector_t);
    subsectors    = (subsector_t *)Z_Malloc(numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
    data          = (uint8_t *)W_CacheLumpNum(lump, PU_STATIC);

    memset(subsectors, 0, numsubsectors * sizeof(subsector_t));

    for (i = 0; i < numsubsectors; i++)
    {
        subsectors[i].numlines  = (uint16_t)Poco::ByteOrder::fromLittleEndian(((mapsubsector_t *)data)[i].numsegs);
        subsectors[i].firstline = (uint16_t)Poco::ByteOrder::fromLittleEndian(((mapsubsector_t *)data)[i].firstseg);
    }

    Z_Free(data);
}

//
// P_LoadSectors
//
void P_LoadSectors(int32_t lump)
{
    uint8_t     *data;
    int32_t      i;
    mapsector_t *ms;
    sector_t    *ss;
    int32_t      defSeqType;

    // denis - properly destroy sectors so that smart pointers they contain don't get screwed
    delete[] sectors;
    originalLightLevels.clear();

    numsectors = W_LumpLength(lump) / sizeof(mapsector_t);

    // denis - properly construct sectors so that smart pointers they contain don't get screwed
    sectors = new sector_t[numsectors];
    memset(sectors, 0, sizeof(sector_t) * numsectors);

    data = (uint8_t *)W_CacheLumpNum(lump, PU_STATIC);

    if (level.flags & LEVEL_SNDSEQTOTALCTRL)
        defSeqType = 0;
    else
        defSeqType = -1;

    ms = (mapsector_t *)data;
    ss = sectors;
    for (i = 0; i < numsectors; i++, ss++, ms++)
    {
        ss->floorheight   = Poco::ByteOrder::fromLittleEndian(ms->floorheight) << FRACBITS;
        ss->ceilingheight = Poco::ByteOrder::fromLittleEndian(ms->ceilingheight) << FRACBITS;
        ss->floorpic      = texturemanager.getHandle(ms->floorpic, Texture::TEX_FLAT);
        ss->ceilingpic    = texturemanager.getHandle(ms->ceilingpic, Texture::TEX_FLAT);
        ss->lightlevel    = Poco::ByteOrder::fromLittleEndian(ms->lightlevel);
        originalLightLevels.push_back(Poco::ByteOrder::fromLittleEndian(ms->lightlevel));
        ss->special            = Poco::ByteOrder::fromLittleEndian(ms->special);
        ss->secretsector       = !!(ss->special & SECRET_MASK);
        ss->tag                = Poco::ByteOrder::fromLittleEndian(ms->tag);
        ss->thinglist          = NULL;
        ss->touching_thinglist = NULL; // phares 3/14/98
        ss->seqType            = defSeqType;
        ss->nextsec            = -1;   // jff 2/26/98 add fields to support locking out
        ss->prevsec            = -1;   // stair retriggering until build completes

        // damage
        ss->damageamount   = 0;
        ss->damageinterval = 0;
        ss->leakrate       = 0;

        // killough 3/7/98:
        ss->floor_xoffs   = 0;
        ss->floor_yoffs   = 0; // floor and ceiling flats offsets
        ss->ceiling_xoffs = 0;
        ss->ceiling_yoffs = 0;

        ss->floor_xscale   = FRACUNIT; // [RH] floor and ceiling scaling
        ss->floor_yscale   = FRACUNIT;
        ss->ceiling_xscale = FRACUNIT;
        ss->ceiling_yscale = FRACUNIT;

        ss->floor_angle   = 0; // [RH] floor and ceiling rotation
        ss->ceiling_angle = 0;

        ss->base_ceiling_angle = ss->base_ceiling_yoffs = ss->base_floor_angle = ss->base_floor_yoffs = 0;

        ss->heightsec     = NULL; // sector used to get floor and ceiling height
        ss->floorlightsec = NULL; // sector used to get floor lighting
        // killough 3/7/98: end changes

        // killough 4/11/98 sector used to get ceiling lighting:
        ss->ceilinglightsec = NULL;

        // [SL] 2012-01-17 - init the sector's floor and ceiling planes
        // as level planes (constant value of z for all points)
        // Slopes will be setup later
        P_SetupLevelFloorPlane(ss);
        P_SetupLevelCeilingPlane(ss);

        ss->gravity = 1.0f; // [RH] Default sector gravity of 1.0

        // [RH] Sectors default to white light with the default fade.
        //		If they are outside (have a sky ceiling), they use the outside fog.
        // [SL] no fog is indicated by outsidefog_color == 0xFF, 0, 0, 0
        bool fog = level.outsidefog_color[0] != 0xFF || level.outsidefog_color[1] != 0 ||
                   level.outsidefog_color[2] != 0 || level.outsidefog_color[3] != 0;

        if (fog && ss->ceilingpic == skyflatnum)
            ss->colormap = GetSpecialLights(255, 255, 255, level.outsidefog_color[1], level.outsidefog_color[2],
                                            level.outsidefog_color[3]);
        else
            ss->colormap = &NormalLight;

        ss->sky = 0;

        // killough 8/28/98: initialize all sectors to normal friction
        ss->friction   = ORIG_FRICTION;
        ss->movefactor = ORIG_FRICTION_FACTOR;
    }

    Z_Free(data);
}

//
// P_LoadNodes
//
void P_LoadNodes(int32_t lump)
{
    if (!W_LumpLength(lump))
    {
        I_Error("P_LoadNodes: NODES lump is empty - levels without nodes are not supported.");
    }

    uint8_t   *data;
    int32_t    i;
    int32_t    j;
    int32_t    k;
    mapnode_t *mn;
    node_t    *no;

    numnodes = W_LumpLength(lump) / sizeof(mapnode_t);
    nodes    = (node_t *)Z_Malloc(numnodes * sizeof(node_t), PU_LEVEL, 0);
    data     = (uint8_t *)W_CacheLumpNum(lump, PU_STATIC);

    mn = (mapnode_t *)data;
    no = nodes;

    for (i = 0; i < numnodes; i++, no++, mn++)
    {
        no->x  = Poco::ByteOrder::fromLittleEndian(mn->x) << FRACBITS;
        no->y  = Poco::ByteOrder::fromLittleEndian(mn->y) << FRACBITS;
        no->dx = Poco::ByteOrder::fromLittleEndian(mn->dx) << FRACBITS;
        no->dy = Poco::ByteOrder::fromLittleEndian(mn->dy) << FRACBITS;
        for (j = 0; j < 2; j++)
        {
            // account for children's promotion to 32 bits
            uint32_t child = (uint16_t)Poco::ByteOrder::fromLittleEndian(mn->children[j]);

            if (child == 0xffff)
                child = 0xffffffff;
            else if (child & 0x8000)
                child = (child & ~0x8000) | NF_SUBSECTOR;

            no->children[j] = child;

            for (k = 0; k < 4; k++)
                no->bbox[j][k] = Poco::ByteOrder::fromLittleEndian(mn->bbox[j][k]) << FRACBITS;
        }
    }

    Z_Free(data);
}

//
// P_LoadXNOD - load ZDBSP extended nodes
// returns false if nodes are not extended to fall back to original nodes
//
bool P_LoadXNOD(int32_t lump)
{
    size_t   len  = W_LumpLength(lump);
    uint8_t *data = (uint8_t *)W_CacheLumpNum(lump, PU_STATIC);

    if (len < 4 || memcmp(data, "XNOD", 4) != 0)
    {
        Z_Free(data);
        return false;
    }

    uint8_t *p = data + 4; // skip the magic number

    // Load vertices
    uint32_t numorgvert = Poco::ByteOrder::fromLittleEndian(*(uint32_t *)p);
    p += 4;
    uint32_t numnewvert = Poco::ByteOrder::fromLittleEndian(*(uint32_t *)p);
    p += 4;

    vertex_t *newvert = (vertex_t *)Z_Malloc((numorgvert + numnewvert) * sizeof(*newvert), PU_LEVEL, 0);

    memcpy(newvert, vertexes, numorgvert * sizeof(*newvert));
    memset(&newvert[numorgvert], 0, numnewvert * sizeof(*newvert));

    for (uint32_t i = 0; i < numnewvert; i++)
    {
        vertex_t *v = &newvert[numorgvert + i];
        v->x        = Poco::ByteOrder::fromLittleEndian(*(int32_t *)p);
        p += 4;
        v->y = Poco::ByteOrder::fromLittleEndian(*(int32_t *)p);
        p += 4;
    }

    // Adjust linedefs - since we reallocated the vertex array,
    // all vertex pointers in linedefs must be updated

    for (int32_t i = 0; i < numlines; i++)
    {
        lines[i].v1 = newvert + (lines[i].v1 - vertexes);
        lines[i].v2 = newvert + (lines[i].v2 - vertexes);
    }

    // nuke the old list, update globals to point to the new list
    Z_Free(vertexes);
    vertexes    = newvert;
    numvertexes = numorgvert + numnewvert;

    // Load subsectors

    numsubsectors = Poco::ByteOrder::fromLittleEndian(*(uint32_t *)p);
    p += 4;
    subsectors = (subsector_t *)Z_Malloc(numsubsectors * sizeof(*subsectors), PU_LEVEL, 0);
    memset(subsectors, 0, numsubsectors * sizeof(*subsectors));

    uint32_t first_seg = 0;

    for (int32_t i = 0; i < numsubsectors; i++)
    {
        subsectors[i].firstline = first_seg;
        subsectors[i].numlines  = Poco::ByteOrder::fromLittleEndian(*(uint32_t *)p);
        p += 4;
        first_seg += subsectors[i].numlines;
    }

    // Load segs

    numsegs = Poco::ByteOrder::fromLittleEndian(*(uint32_t *)p);
    p += 4;
    segs = (seg_t *)Z_Malloc(numsegs * sizeof(*segs), PU_LEVEL, 0);
    memset(segs, 0, numsegs * sizeof(*segs));

    for (int32_t i = 0; i < numsegs; i++)
    {
        uint32_t v1 = Poco::ByteOrder::fromLittleEndian(*(uint32_t *)p);
        p += 4;
        uint32_t v2 = Poco::ByteOrder::fromLittleEndian(*(uint32_t *)p);
        p += 4;
        uint16_t ld = Poco::ByteOrder::fromLittleEndian(*(uint16_t *)p);
        p += 2;
        uint8_t side = *(uint8_t *)p;
        p += 1;

        if (side != 0 && side != 1)
            side = 1;

        seg_t  *seg  = &segs[i];
        line_t *line = &lines[ld];

        seg->v1 = &vertexes[v1];
        seg->v2 = &vertexes[v2];

        seg->linedef = line;
        seg->sidedef = &sides[line->sidenum[side]];

        seg->frontsector = seg->sidedef->sector;
        if (line->flags & ML_TWOSIDED && line->sidenum[side ^ 1] != R_NOSIDE)
            seg->backsector = sides[line->sidenum[side ^ 1]].sector;
        else
            seg->backsector = NULL;

        seg->angle = R_PointToAngle2(seg->v1->x, seg->v1->y, seg->v2->x, seg->v2->y);

        // a short version of the offset calculation in P_LoadSegs
        vertex_t *origin = (side == 0) ? line->v1 : line->v2;
        float     dx     = FIXED2FLOAT(seg->v1->x - origin->x);
        float     dy     = FIXED2FLOAT(seg->v1->y - origin->y);
        seg->offset      = FLOAT2FIXED(sqrt(dx * dx + dy * dy));
    }

    // Load nodes

    numnodes = Poco::ByteOrder::fromLittleEndian(*(uint32_t *)p);
    p += 4;
    nodes = (node_t *)Z_Malloc(numnodes * sizeof(*nodes), PU_LEVEL, 0);
    memset(nodes, 0, numnodes * sizeof(*nodes));

    for (int32_t i = 0; i < numnodes; i++)
    {
        node_t *node = &nodes[i];

        node->x = Poco::ByteOrder::fromLittleEndian(*(int16_t *)p) << FRACBITS;
        p += 2;
        node->y = Poco::ByteOrder::fromLittleEndian(*(int16_t *)p) << FRACBITS;
        p += 2;
        node->dx = Poco::ByteOrder::fromLittleEndian(*(int16_t *)p) << FRACBITS;
        p += 2;
        node->dy = Poco::ByteOrder::fromLittleEndian(*(int16_t *)p) << FRACBITS;
        p += 2;

        for (int32_t j = 0; j < 2; j++)
        {
            for (int32_t k = 0; k < 4; k++)
            {
                node->bbox[j][k] = Poco::ByteOrder::fromLittleEndian(*(int16_t *)p) << FRACBITS;
                p += 2;
            }
        }

        for (int32_t j = 0; j < 2; j++)
        {
            node->children[j] = Poco::ByteOrder::fromLittleEndian(*(uint32_t *)p);
            p += 4;
        }
    }

    Z_Free(data);

    return true;
}

//
// P_LoadThings
//
void P_LoadThings(int32_t lump)
{
    mapthing2_t mt2; // [RH] for translation
    uint8_t    *data   = (uint8_t *)W_CacheLumpNum(lump, PU_STATIC);
    mapthing_t *mt     = (mapthing_t *)data;
    mapthing_t *lastmt = (mapthing_t *)(data + W_LumpLength(lump));

    playerstarts.clear();
    voodoostarts.clear();
    DeathMatchStarts.clear();
    for (int32_t iTeam = 0; iTeam < NUMTEAMS; iTeam++)
        GetTeamInfo((team_t)iTeam)->Starts.clear();

    // [RH] ZDoom now uses Hexen-style maps as its native format. // denis - growwwwl
    //		Since this is the only place where Doom-style Things are ever
    //		referenced, we translate them into a Hexen-style thing.
    for (; mt < lastmt; mt++)
    {
        // [AM] Ensure that we get a fresh mapthing every iteration - sometimes
        //      P_SpawnMapThing mutates a part of the mapthing that the map
        //      data doesn't care about, and we don't want it to carry over
        //      between iterations.
        memset(&mt2, 0, sizeof(mt2));

        // [RH] At this point, monsters unique to Doom II were weeded out
        //		if the IWAD wasn't for Doom II. R_SpawnMapThing() can now
        //		handle these and more cases better, so we just pass it
        //		everything and let it decide what to do with them.

        // [RH] Need to translate the spawn flags to Hexen format.
        int16_t flags = Poco::ByteOrder::fromLittleEndian(mt->options);
        mt2.flags     = (int16_t)((flags & 0xf) | 0x7e0);
        if (flags & BTF_NOTSINGLE)
        {
#ifdef SERVER_APP
            if (G_IsCoopGame())
            {
                if (g_thingfilter == 1)
                    mt2.flags |= MTF_FILTER_COOPWPN;
                else if (g_thingfilter == 2)
                    mt2.flags &= ~MTF_COOPERATIVE;
            }
            else
#endif
                mt2.flags &= ~MTF_SINGLE;
        }
        if (flags & BTF_NOTDEATHMATCH)
            mt2.flags &= ~MTF_DEATHMATCH;
        if (flags & BTF_NOTCOOPERATIVE)
            mt2.flags &= ~MTF_COOPERATIVE;

        mt2.x     = Poco::ByteOrder::fromLittleEndian(mt->x);
        mt2.y     = Poco::ByteOrder::fromLittleEndian(mt->y);
        mt2.angle = Poco::ByteOrder::fromLittleEndian(mt->angle);
        mt2.type  = Poco::ByteOrder::fromLittleEndian(mt->type);

        P_SpawnMapThing(&mt2, 0);
    }

    P_SpawnAvatars();

    Z_Free(data);
}

// [RH]
// P_LoadThings2
//
// Same as P_LoadThings() except it assumes Things are
// saved Hexen-style. Position also controls which single-
// player start spots are spawned by filtering out those
// whose first parameter don't match position.
//
void P_LoadThings2(int32_t lump, int32_t position)
{
    uint8_t     *data   = (uint8_t *)W_CacheLumpNum(lump, PU_STATIC);
    mapthing2_t *mt     = (mapthing2_t *)data;
    mapthing2_t *lastmt = (mapthing2_t *)(data + W_LumpLength(lump));

    playerstarts.clear();
    voodoostarts.clear();
    DeathMatchStarts.clear();
    for (int32_t iTeam = 0; iTeam < NUMTEAMS; iTeam++)
        GetTeamInfo((team_t)iTeam)->Starts.clear();

    for (; mt < lastmt; mt++)
    {
        // [RH] At this point, monsters unique to Doom II were weeded out
        //		if the IWAD wasn't for Doom II. R_SpawnMapThing() can now
        //		handle these and more cases better, so we just pass it
        //		everything and let it decide what to do with them.

        mt->thingid = Poco::ByteOrder::fromLittleEndian(mt->thingid);
        mt->x       = Poco::ByteOrder::fromLittleEndian(mt->x);
        mt->y       = Poco::ByteOrder::fromLittleEndian(mt->y);
        mt->z       = Poco::ByteOrder::fromLittleEndian(mt->z);
        mt->angle   = Poco::ByteOrder::fromLittleEndian(mt->angle);
        mt->type    = Poco::ByteOrder::fromLittleEndian(mt->type);
        mt->flags   = Poco::ByteOrder::fromLittleEndian(mt->flags);

        P_SpawnMapThing(mt, position);
    }

    Z_Free(data);
}

//
// P_LoadLineDefs
//
// killough 4/4/98: split into two functions, to allow sidedef overloading
//
// [RH] Actually split into four functions to allow for Hexen and Doom
//		linedefs.
void P_AdjustLine(line_t *ld)
{
    vertex_t *v1, *v2;

    ld->lucency = 255; // [RH] Opaque by default

    v1 = ld->v1;
    v2 = ld->v2;

    ld->dx = v2->x - v1->x;
    ld->dy = v2->y - v1->y;

    if (ld->dx == 0)
        ld->slopetype = ST_VERTICAL;
    else if (ld->dy == 0)
        ld->slopetype = ST_HORIZONTAL;
    else
        ld->slopetype = (FixedDiv(ld->dy, ld->dx) > 0) ? ST_POSITIVE : ST_NEGATIVE;

    if (v1->x < v2->x)
    {
        ld->bbox[BOXLEFT]  = v1->x;
        ld->bbox[BOXRIGHT] = v2->x;
    }
    else
    {
        ld->bbox[BOXLEFT]  = v2->x;
        ld->bbox[BOXRIGHT] = v1->x;
    }

    if (v1->y < v2->y)
    {
        ld->bbox[BOXBOTTOM] = v1->y;
        ld->bbox[BOXTOP]    = v2->y;
    }
    else
    {
        ld->bbox[BOXBOTTOM] = v2->y;
        ld->bbox[BOXTOP]    = v1->y;
    }

    if (map_format.getZDoom())
    {
        // [RH] Set line id (as appropriate) here
        if (ld->special == Line_SetIdentification || ld->special == Teleport_Line || ld->special == TranslucentLine ||
            ld->special == Scroll_Texture_Model)
        {
            ld->id = ld->args[0];
        }
    }
    else
    {
        if (P_IsThingNoFogTeleportLine(ld->special))
        {
            if (ld->id == 0)
            {
                // Untagged teleporters teleport to tid 1.
                ld->args[0] = 1;
            }
            else
            {
                ld->args[2] = ld->id;
                ld->args[0] = 0;
            }
        }
        else if (ld->special >= OdamexStaticInits && ld->special < OdamexStaticInits + NUM_STATIC_INITS)
        {
            // An Odamex Static_Init special
            ld->args[0] = ld->id;
            ld->args[1] = ld->special - OdamexStaticInits;
        }
        else if (ld->special >= 340 && ld->special <= 347)
        {
            // [SL] 2012-01-30 - convert to ZDoom Plane_Align special for
            // sloping sectors
            // [Blair] Massage this a bit to work with map_format
            switch (ld->special)
            {
            case 340: // Slope the Floor in front of the line
                ld->args[0] = 1;
                break;
            case 341: // Slope the Ceiling in front of the line
                ld->args[1] = 1;
                break;
            case 342: // Slope the Floor+Ceiling in front of the line
                ld->args[0] = ld->args[1] = 1;
                break;
            case 343: // Slope the Floor behind the line
                ld->args[0] = 2;
                break;
            case 344: // Slope the Ceiling behind the line
                ld->args[1] = 2;
                break;
            case 345: // Slope the Floor+Ceiling behind the line
                ld->args[0] = ld->args[1] = 2;
                break;
            case 346: // Slope the Floor behind+Ceiling in front of the line
                ld->args[0] = 2;
                ld->args[1] = 1;
                break;
            case 347: // Slope the Floor in front+Ceiling behind the line
                ld->args[0] = 1;
                ld->args[1] = 2;
            }
        }
    }

    // denis - prevent buffer overrun
    if (*ld->sidenum == R_NOSIDE)
        return;

    if (map_format.getZDoom())
    {
        // killough 4/4/98: support special sidedef interpretation below
        if ( // [RH] Save Static_Init only if it's interested in the textures
            ((ld->special == Static_Init && ld->args[1] == Init_Color) || ld->special != Static_Init))
        {
            sides[*ld->sidenum].special = ld->special;
            sides[*ld->sidenum].tag     = ld->args[0];
        }
        else
        {
            sides[*ld->sidenum].special = 0;
        }
    }
    else
    {
        // killough 4/4/98: support special sidedef interpretation below
        if (ld->special >= OdamexStaticInits + 1 || ld->special <= OdamexStaticInits + NUM_STATIC_INITS)
        {
            sides[*ld->sidenum].special = ld->special;
            sides[*ld->sidenum].tag     = ld->args[0];
        }
        else
        {
            sides[*ld->sidenum].special = 0;
        }
    }
}

// killough 4/4/98: delay using sidedefs until they are loaded
void P_FinishLoadingLineDefs(void)
{
    int32_t i, linenum;
    line_t *ld = lines;

    for (i = numlines, linenum = 0; i--; ld++, linenum++)
    {
        ld->frontsector = ld->sidenum[0] != R_NOSIDE ? sides[ld->sidenum[0]].sector : 0;
        ld->backsector  = ld->sidenum[1] != R_NOSIDE ? sides[ld->sidenum[1]].sector : 0;
        if (ld->sidenum[0] != R_NOSIDE)
            sides[ld->sidenum[0]].linenum = linenum;
        if (ld->sidenum[1] != R_NOSIDE)
            sides[ld->sidenum[1]].linenum = linenum;

        map_format.post_process_linedef_special(ld);
    }
}

void P_LoadLineDefs(const int32_t lump)
{
    uint8_t *data;
    int32_t  i;
    line_t  *ld;

    numlines = W_LumpLength(lump) / sizeof(maplinedef_t);
    lines    = (line_t *)Z_Malloc(numlines * sizeof(line_t), PU_LEVEL, 0);
    memset(lines, 0, numlines * sizeof(line_t));
    data = (uint8_t *)W_CacheLumpNum(lump, PU_STATIC);

    // [Blair] Don't mind me, just hackin'
    // E2M7 has flags masked in that interfere with MBF21 flags.
    // Boom fixes this with the comp flag comp_reservedlineflag
    // We'll fix this for now by just checking for the E2M7 FarmHash
    const std::string e2m7hash = "43ffa244f5ae923b7df59dbf511c0468";

    std::string levelHash;

    // [Blair] Serialize the hashes before reading.
    uint64_t reconsthash1 =
        (uint64_t)(::level.level_fingerprint[0]) | (uint64_t)(::level.level_fingerprint[1]) << 8 |
        (uint64_t)(::level.level_fingerprint[2]) << 16 | (uint64_t)(::level.level_fingerprint[3]) << 24 |
        (uint64_t)(::level.level_fingerprint[4]) << 32 | (uint64_t)(::level.level_fingerprint[5]) << 40 |
        (uint64_t)(::level.level_fingerprint[6]) << 48 | (uint64_t)(::level.level_fingerprint[7]) << 56;

    uint64_t reconsthash2 =
        (uint64_t)(::level.level_fingerprint[8]) | (uint64_t)(::level.level_fingerprint[9]) << 8 |
        (uint64_t)(::level.level_fingerprint[10]) << 16 | (uint64_t)(::level.level_fingerprint[11]) << 24 |
        (uint64_t)(::level.level_fingerprint[12]) << 32 | (uint64_t)(::level.level_fingerprint[13]) << 40 |
        (uint64_t)(::level.level_fingerprint[14]) << 48 | (uint64_t)(::level.level_fingerprint[15]) << 56;

    StrFormat(levelHash, "%16llx%16llx", reconsthash1, reconsthash2);

    bool isE2M7 = (levelHash == e2m7hash);

    ld = lines;
    for (i = 0; i < numlines; i++, ld++)
    {
        const maplinedef_t *mld = ((maplinedef_t *)data) + i;

        ld->flags   = (uint16_t)(int16_t)mld->flags;
        ld->special = (int16_t)mld->special;
        ld->id      = (int16_t)mld->tag;
        ld->args[0] = 0;
        ld->args[1] = 0;
        ld->args[2] = 0;
        ld->args[3] = 0;
        ld->args[4] = 0;

        ld->flags = P_TranslateCompatibleLineFlags(ld->flags, isE2M7);

        uint16_t v = Poco::ByteOrder::fromLittleEndian(mld->v1);

        if (v >= numvertexes)
            I_Error("P_LoadLineDefs: invalid vertex %d", v);
        else
            ld->v1 = &vertexes[v];

        v = Poco::ByteOrder::fromLittleEndian(mld->v2);

        if (v >= numvertexes)
            I_Error("P_LoadLineDefs: invalid vertex %d", v);
        else
            ld->v2 = &vertexes[v];

        ld->sidenum[0] = Poco::ByteOrder::fromLittleEndian(mld->sidenum[0]);
        ld->sidenum[1] = Poco::ByteOrder::fromLittleEndian(mld->sidenum[1]);

        if (ld->sidenum[0] >= numsides)
            ld->sidenum[0] = R_NOSIDE;
        if (ld->sidenum[1] >= numsides)
            ld->sidenum[1] = R_NOSIDE;

        P_AdjustLine(ld);
    }

    Z_Free(data);
}

// [RH] Same as P_LoadLineDefs() except it uses Hexen-style LineDefs.
void P_LoadLineDefs2(int32_t lump)
{
    uint8_t       *data;
    int32_t        i;
    maplinedef2_t *mld;
    line_t        *ld;

    numlines = W_LumpLength(lump) / sizeof(maplinedef2_t);
    lines    = (line_t *)Z_Malloc(numlines * sizeof(line_t), PU_LEVEL, 0);
    memset(lines, 0, numlines * sizeof(line_t));
    data = (uint8_t *)W_CacheLumpNum(lump, PU_STATIC);

    mld = (maplinedef2_t *)data;
    ld  = lines;
    for (i = 0; i < numlines; i++, mld++, ld++)
    {
        int32_t j;

        for (j = 0; j < 5; j++)
            ld->args[j] = mld->args[j];

        ld->flags   = Poco::ByteOrder::fromLittleEndian(mld->flags);
        ld->special = mld->special;

        ld->flags = P_TranslateZDoomLineFlags(ld->flags);

        uint16_t v = Poco::ByteOrder::fromLittleEndian(mld->v1);

        if (v >= numvertexes)
            I_Error("P_LoadLineDefs2: invalid vertex %d", v);
        else
            ld->v1 = &vertexes[v];

        v = Poco::ByteOrder::fromLittleEndian(mld->v2);

        if (v >= numvertexes)
            I_Error("P_LoadLineDefs2: invalid vertex %d", v);
        else
            ld->v2 = &vertexes[v];

        ld->sidenum[0] = Poco::ByteOrder::fromLittleEndian(mld->sidenum[0]);
        ld->sidenum[1] = Poco::ByteOrder::fromLittleEndian(mld->sidenum[1]);

        if (ld->sidenum[0] >= numsides)
            ld->sidenum[0] = R_NOSIDE;
        if (ld->sidenum[1] >= numsides)
            ld->sidenum[1] = R_NOSIDE;

        P_AdjustLine(ld);
    }

    Z_Free(data);
}

//
// P_LoadSideDefs
//
// killough 4/4/98: split into two functions
void P_LoadSideDefs(int32_t lump)
{
    numsides = W_LumpLength(lump) / sizeof(mapsidedef_t);
    sides    = (side_t *)Z_Malloc(numsides * sizeof(side_t), PU_LEVEL, 0);
    memset(sides, 0, numsides * sizeof(side_t));
}

//
// P_GetColorFromTextureName
//
// Converts a texture name to an ARGB8888 value.
// The texture name should contain 4 hexadecimal byte values
// in the following order: alpha, red, green, blue.
//
static argb_t P_GetColorFromTextureName(const char *name)
{
    // work around name not being a properly terminated string
    char name2[9];
    strncpy(name2, name, 8);
    name2[8] = '\0';

    unsigned long value = strtoul(name2, NULL, 16);

    int32_t a = (value >> 24) & 0xFF;
    int32_t r = (value >> 16) & 0xFF;
    int32_t g = (value >> 8) & 0xFF;
    int32_t b = value & 0xFF;

    return argb_t(a, r, g, b);
}

//
// P_SetTransferHeightBlends
//
// Reads the texture name from the mapsidedef for the given side. If the
// texture name matches the name of a valid Boom colormap lump, the
// sidedef's texture value is cleared and the colormap's blend color
// value is used for the appropriate sector blend. If the texture name
// is an ARGB value in hexadecimal, that value is used for the appropriate
// sector blend.
//
void P_SetTransferHeightBlends(side_t *sd, const mapsidedef_t *msd)
{
    sector_t *sec = &sectors[Poco::ByteOrder::fromLittleEndian(msd->sector)];

    // for each of the texture tiers (bottom, middle, and top)
    for (int32_t i = 0; i < 3; i++)
    {
        texhandle_t *texture_num;
        argb_t      *blend_color;
        const char  *texture_name;

        if (i == 0) // bottom textures
        {
            texture_num  = &sd->bottomtexture;
            blend_color  = &sec->bottommap;
            texture_name = msd->bottomtexture;
        }
        else if (i == 1) // mid textures
        {
            texture_num  = &sd->midtexture;
            blend_color  = &sec->midmap;
            texture_name = msd->midtexture;
        }
        else // top textures
        {
            texture_num  = &sd->toptexture;
            blend_color  = &sec->topmap;
            texture_name = msd->toptexture;
        }

        *blend_color = argb_t(0, 255, 255, 255);
        *texture_num = texturemanager.getHandle(texture_name, Texture::TEX_TEXTURE);
        if (*texture_num == TextureManager::NOT_FOUND_TEXTURE_HANDLE)
        {
            *texture_num = TextureManager::NO_TEXTURE_HANDLE;
            if (strnicmp(texture_name, "WATERMAP", 8) == 0)
                *blend_color = argb_t(0x80, 0, 0x4F, 0xA5);
            else
                *blend_color = P_GetColorFromTextureName(texture_name);
        }
    }
}

//

void SetTextureNoErr(texhandle_t *texture, uint32_t *color, char *name)
{
    if ((*texture = texturemanager.getHandle(name, Texture::TEX_TEXTURE)) == TextureManager::NOT_FOUND_TEXTURE_HANDLE)
    {
        char  name2[9];
        char *stop;
        strncpy(name2, name, 8);
        name2[8] = 0;
        *color   = strtoul(name2, &stop, 16);
        *texture = TextureManager::NO_TEXTURE_HANDLE;
    }
}

// killough 4/4/98: delay using texture names until
// after linedefs are loaded, to allow overloading.
// killough 5/3/98: reformatted, cleaned up

void P_LoadSideDefs2(int32_t lump)
{
    uint8_t *data = (uint8_t *)W_CacheLumpNum(lump, PU_STATIC);

    for (int32_t i = 0; i < numsides; i++)
    {
        mapsidedef_t *msd = (mapsidedef_t *)data + i;
        side_t       *sd  = sides + i;
        sector_t     *sec;

        sd->textureoffset = Poco::ByteOrder::fromLittleEndian(msd->textureoffset) << FRACBITS;
        sd->rowoffset     = Poco::ByteOrder::fromLittleEndian(msd->rowoffset) << FRACBITS;
        sd->linenum       = -1;
        sd->sector = sec = &sectors[Poco::ByteOrder::fromLittleEndian(msd->sector)];

        // killough 4/4/98: allow sidedef texture names to be overloaded
        // killough 4/11/98: refined to allow colormaps to work as wall
        // textures if invalid as colormaps but valid as textures.

        map_format.post_process_sidedef_special(sd, msd, sec, i);
    }
    Z_Free(data);
}

//
// jff 10/6/98
// New code added to speed up calculation of internal blockmap
// Algorithm is order of nlines*(ncols+nrows) not nlines*ncols*nrows
//

#define blkshift  7                     /* places to shift rel position for cell num */
#define blkmask   ((1 << blkshift) - 1) /* mask for rel position within cell */
#define blkmargin 0                     /* size guardband around map used */
                                        // jff 10/8/98 use guardband>0
                                        // jff 10/12/98 0 ok with + 1 in rows,cols

typedef struct linelist_t               // type used to list lines in each block
{
    int32_t            num;
    struct linelist_t *next;
} linelist_t;

//
// Subroutine to add a line number to a block list
// It simply returns if the line is already in the block
//

static void AddBlockLine(linelist_t **lists, int32_t *count, int32_t *done, int32_t blockno, uint32_t lineno)
{
    linelist_t *l;

    if (done[blockno])
        return;

    l              = new linelist_t;
    l->num         = lineno;
    l->next        = lists[blockno];
    lists[blockno] = l;
    count[blockno]++;
    done[blockno] = 1;
}

//
// Actually construct the blockmap lump from the level data
//
// This finds the intersection of each linedef with the column and
// row lines at the left and bottom of each blockmap cell. It then
// adds the line to all block lists touching the intersection.
//

void P_CreateBlockMap()
{
    int32_t      xorg, yorg;           // blockmap origin (lower left)
    int32_t      nrows, ncols;         // blockmap dimensions
    linelist_t **blocklists = NULL;    // array of pointers to lists of lines
    int32_t     *blockcount = NULL;    // array of counters of line lists
    int32_t     *blockdone  = NULL;    // array keeping track of blocks/line
    int32_t      NBlocks;              // number of cells = nrows*ncols
    uint32_t     linetotal = 0;        // total length of all blocklists
    int32_t      i, j;
    int32_t      map_minx = INT32_MAX; // init for map limits search
    int32_t      map_miny = INT32_MAX;
    int32_t      map_maxx = INT32_MIN;
    int32_t      map_maxy = INT32_MIN;

    // scan for map limits, which the blockmap must enclose

    for (i = 0; i < numvertexes; i++)
    {
        fixed_t t;

        if ((t = vertexes[i].x) < map_minx)
            map_minx = t;
        else if (t > map_maxx)
            map_maxx = t;
        if ((t = vertexes[i].y) < map_miny)
            map_miny = t;
        else if (t > map_maxy)
            map_maxy = t;
    }
    map_minx >>= FRACBITS; // work in map coords, not fixed_t
    map_maxx >>= FRACBITS;
    map_miny >>= FRACBITS;
    map_maxy >>= FRACBITS;

    // set up blockmap area to enclose level plus margin

    xorg    = map_minx - blkmargin;
    yorg    = map_miny - blkmargin;
    ncols   = (map_maxx + blkmargin - xorg + 1 + blkmask) >> blkshift; // jff 10/12/98
    nrows   = (map_maxy + blkmargin - yorg + 1 + blkmask) >> blkshift; //+1 needed for
    NBlocks = ncols * nrows;                                           // map exactly 1 cell

    // create the array of pointers on NBlocks to blocklists
    // also create an array of linelist counts on NBlocks
    // finally make an array in which we can mark blocks done per line

    blocklists = new linelist_t *[NBlocks];
    memset(blocklists, 0, NBlocks * sizeof(linelist_t *));
    blockcount = new int32_t[NBlocks];
    memset(blockcount, 0, NBlocks * sizeof(int32_t));
    blockdone = new int32_t[NBlocks];

    // initialize each blocklist, and enter the trailing -1 in all blocklists
    // note the linked list of lines grows backwards

    for (i = 0; i < NBlocks; i++)
    {
        blocklists[i]       = new linelist_t;
        blocklists[i]->num  = -1;
        blocklists[i]->next = NULL;
        blockcount[i]++;
    }

    // For each linedef in the wad, determine all blockmap blocks it touches,
    // and add the linedef number to the blocklists for those blocks

    for (i = 0; i < numlines; i++)
    {
        int32_t x1    = lines[i].v1->x >> FRACBITS; // lines[i] map coords
        int32_t y1    = lines[i].v1->y >> FRACBITS;
        int32_t x2    = lines[i].v2->x >> FRACBITS;
        int32_t y2    = lines[i].v2->y >> FRACBITS;
        int32_t dx    = x2 - x1;
        int32_t dy    = y2 - y1;
        int32_t vert  = !dx; // lines[i] slopetype
        int32_t horiz = !dy;
        int32_t spos  = (dx ^ dy) > 0;
        int32_t sneg  = (dx ^ dy) < 0;
        int32_t bx, by;                   // block cell coords
        int32_t minx = x1 > x2 ? x2 : x1; // extremal lines[i] coords
        int32_t maxx = x1 > x2 ? x1 : x2;
        int32_t miny = y1 > y2 ? y2 : y1;
        int32_t maxy = y1 > y2 ? y1 : y2;

        // no blocks done for this linedef yet

        memset(blockdone, 0, NBlocks * sizeof(int32_t));

        // The line always belongs to the blocks containing its endpoints

        bx = (x1 - xorg) >> blkshift;
        by = (y1 - yorg) >> blkshift;
        AddBlockLine(blocklists, blockcount, blockdone, by * ncols + bx, i);
        bx = (x2 - xorg) >> blkshift;
        by = (y2 - yorg) >> blkshift;
        AddBlockLine(blocklists, blockcount, blockdone, by * ncols + bx, i);

        // For each column, see where the line along its left edge, which
        // it contains, intersects the Linedef i. Add i to each corresponding
        // blocklist.

        if (!vert) // don't interesect vertical lines with columns
        {
            for (j = 0; j < ncols; j++)
            {
                // intersection of Linedef with x=xorg+(j<<blkshift)
                // (y-y1)*dx = dy*(x-x1)
                // y = dy*(x-x1)+y1*dx;

                int32_t x  = xorg + (j << blkshift); // (x,y) is intersection
                int32_t y  = (dy * (x - x1)) / dx + y1;
                int32_t yb = (y - yorg) >> blkshift; // block row number
                int32_t yp = (y - yorg) & blkmask;   // y position within block

                if (yb < 0 || yb > nrows - 1)        // outside blockmap, continue
                    continue;

                if (x < minx || x > maxx)            // line doesn't touch column
                    continue;

                // The cell that contains the intersection point is always added

                AddBlockLine(blocklists, blockcount, blockdone, ncols * yb + j, i);

                // if the intersection is at a corner it depends on the slope
                // (and whether the line extends past the intersection) which
                // blocks are hit

                if (yp == 0)  // intersection at a corner
                {
                    if (sneg) //   \ - blocks x,y-, x-,y
                    {
                        if (yb > 0 && miny < y)
                            AddBlockLine(blocklists, blockcount, blockdone, ncols * (yb - 1) + j, i);
                        if (j > 0 && minx < x)
                            AddBlockLine(blocklists, blockcount, blockdone, ncols * yb + j - 1, i);
                    }
                    else if (spos) //   / - block x-,y-
                    {
                        if (yb > 0 && j > 0 && minx < x)
                            AddBlockLine(blocklists, blockcount, blockdone, ncols * (yb - 1) + j - 1, i);
                    }
                    else if (horiz) //   - - block x-,y
                    {
                        if (j > 0 && minx < x)
                            AddBlockLine(blocklists, blockcount, blockdone, ncols * yb + j - 1, i);
                    }
                }
                else if (j > 0 && minx < x) // else not at corner: x-,y
                    AddBlockLine(blocklists, blockcount, blockdone, ncols * yb + j - 1, i);
            }
        }

        // For each row, see where the line along its bottom edge, which
        // it contains, intersects the Linedef i. Add i to all the corresponding
        // blocklists.

        if (!horiz)
        {
            for (j = 0; j < nrows; j++)
            {
                // intersection of Linedef with y=yorg+(j<<blkshift)
                // (x,y) on Linedef i satisfies: (y-y1)*dx = dy*(x-x1)
                // x = dx*(y-y1)/dy+x1;

                int32_t y  = yorg + (j << blkshift); // (x,y) is intersection
                int32_t x  = (dx * (y - y1)) / dy + x1;
                int32_t xb = (x - xorg) >> blkshift; // block column number
                int32_t xp = (x - xorg) & blkmask;   // x position within block

                if (xb < 0 || xb > ncols - 1)        // outside blockmap, continue
                    continue;

                if (y < miny || y > maxy)            // line doesn't touch row
                    continue;

                // The cell that contains the intersection point is always added

                AddBlockLine(blocklists, blockcount, blockdone, ncols * j + xb, i);

                // if the intersection is at a corner it depends on the slope
                // (and whether the line extends past the intersection) which
                // blocks are hit

                if (xp == 0)  // intersection at a corner
                {
                    if (sneg) //   \ - blocks x,y-, x-,y
                    {
                        if (j > 0 && miny < y)
                            AddBlockLine(blocklists, blockcount, blockdone, ncols * (j - 1) + xb, i);
                        if (xb > 0 && minx < x)
                            AddBlockLine(blocklists, blockcount, blockdone, ncols * j + xb - 1, i);
                    }
                    else if (vert) //   | - block x,y-
                    {
                        if (j > 0 && miny < y)
                            AddBlockLine(blocklists, blockcount, blockdone, ncols * (j - 1) + xb, i);
                    }
                    else if (spos) //   / - block x-,y-
                    {
                        if (xb > 0 && j > 0 && miny < y)
                            AddBlockLine(blocklists, blockcount, blockdone, ncols * (j - 1) + xb - 1, i);
                    }
                }
                else if (j > 0 && miny < y) // else not on a corner: x,y-
                    AddBlockLine(blocklists, blockcount, blockdone, ncols * (j - 1) + xb, i);
            }
        }
    }

    // Add initial 0 to all blocklists
    // count the total number of lines (and 0's and -1's)
    memset(blockdone, 0, NBlocks * sizeof(int32_t));
    for (i = 0, linetotal = 0; i < NBlocks; i++)
    {
        AddBlockLine(blocklists, blockcount, blockdone, i, 0);
        linetotal += blockcount[i];
    }

    // Create the blockmap lump
    blockmaplump = (int32_t *)Z_Malloc(sizeof(*blockmaplump) * (4 + NBlocks + linetotal), PU_LEVEL, 0);

    // blockmap header
    //
    // Rjy: P_CreateBlockMap should not initialise bmaporg{x,y} as P_LoadBlockMap
    // does so again, resulting in their being left-shifted by FRACBITS twice.
    //
    // Thus any map having its blockmap built by the engine would have its
    // origin at (0,0) regardless of where the walls and monsters actually are,
    // breaking all collision detection.
    //
    // Instead have P_CreateBlockMap create blockmaplump only, so that both
    // clauses of the conditional in P_LoadBlockMap have the same effect, and
    // bmap* are only initialised from blockmaplump[0..3] once in the latter.
    //
    blockmaplump[0] = xorg;
    blockmaplump[1] = yorg;
    blockmaplump[2] = ncols;
    blockmaplump[3] = nrows;

    // offsets to lists and block lists
    for (i = 0; i < NBlocks; i++)
    {
        linelist_t *bl   = blocklists[i];
        uint32_t    offs = blockmaplump[4 + i] = // set offset to block's list
            (i ? blockmaplump[4 + i - 1] : 4 + NBlocks) + (i ? blockcount[i - 1] : 0);

        // add the lines in each block's list to the blockmaplump
        // delete each list node as we go

        while (bl)
        {
            linelist_t *tmp      = bl->next;
            blockmaplump[offs++] = bl->num;
            delete bl;
            bl = tmp;
        }
    }

    // free all temporary storage
    delete[] blocklists;
    delete[] blockcount;
    delete[] blockdone;
}

// jff 10/6/98
// End new code added to speed up calculation of internal blockmap

//
// P_LoadBlockMap
//
// [RH] Changed this some
//
void P_LoadBlockMap(int32_t lump)
{
    int32_t count;

    if (Args.CheckParm("-blockmap") || (count = W_LumpLength(lump) / 2) >= 0x10000 || count < 4)
        P_CreateBlockMap();
    else
    {
        int16_t *wadblockmaplump = (int16_t *)W_CacheLumpNum(lump, PU_LEVEL);
        int32_t  i;
        blockmaplump = (int32_t *)Z_Malloc(sizeof(*blockmaplump) * count, PU_LEVEL, 0);

        // killough 3/1/98: Expand wad blockmap into larger internal one,
        // by treating all offsets except -1 as uint32_t and zero-extending
        // them. This potentially doubles the size of blockmaps allowed,
        // because Doom originally considered the offsets as always signed.

        blockmaplump[0] = Poco::ByteOrder::fromLittleEndian(wadblockmaplump[0]);
        blockmaplump[1] = Poco::ByteOrder::fromLittleEndian(wadblockmaplump[1]);
        blockmaplump[2] = (uint32_t)(Poco::ByteOrder::fromLittleEndian(wadblockmaplump[2])) & 0xffff;
        blockmaplump[3] = (uint32_t)(Poco::ByteOrder::fromLittleEndian(wadblockmaplump[3])) & 0xffff;

        for (i = 4; i < count; i++)
        {
            int16_t t       = Poco::ByteOrder::fromLittleEndian(wadblockmaplump[i]); // killough 3/1/98
            blockmaplump[i] = t == -1 ? (uint32_t)0xffffffff : (uint32_t)t & 0xffff;
        }

        Z_Free(wadblockmaplump);
    }

    bmaporgx   = blockmaplump[0] << FRACBITS;
    bmaporgy   = blockmaplump[1] << FRACBITS;
    bmapwidth  = blockmaplump[2];
    bmapheight = blockmaplump[3];

    // clear out mobj chains
    count      = sizeof(*blocklinks) * bmapwidth * bmapheight;
    blocklinks = (AActor **)Z_Malloc(count, PU_LEVEL, 0);
    memset(blocklinks, 0, count);
    blockmap = blockmaplump + 4;
}

/*
 * @brief P_GenerateUniqueMapFingerPrint
 *
 * Creates a unique map fingerprint used to identify a unique map.
 * Based on a few key lumps that makes a map unique.
 *
 * @param maplumpnum - Lump offset number of the specified map
 * If it is, use it as part of the map calculation.
 */

// Use MD5 instead of FarmHash, at least for now - Dasho
void P_GenerateUniqueMapFingerPrint(int32_t maplumpnum)
{
    uint32_t length = 0;

    typedef std::vector<uint8_t> LevelLumps;
    LevelLumps                   levellumps;

    const uint8_t *thingbytes =
        const_cast<const uint8_t *>((const uint8_t *)W_CacheLumpNum(maplumpnum + ML_THINGS, PU_STATIC));
    const uint8_t *lindefbytes =
        const_cast<const uint8_t *>((const uint8_t *)W_CacheLumpNum(maplumpnum + ML_LINEDEFS, PU_STATIC));
    const uint8_t *sidedefbytes =
        const_cast<const uint8_t *>((const uint8_t *)W_CacheLumpNum(maplumpnum + ML_SIDEDEFS, PU_STATIC));
    const uint8_t *vertexbytes =
        const_cast<const uint8_t *>((const uint8_t *)W_CacheLumpNum(maplumpnum + ML_VERTEXES, PU_STATIC));
    const uint8_t *segsbytes =
        const_cast<const uint8_t *>((const uint8_t *)W_CacheLumpNum(maplumpnum + ML_SEGS, PU_STATIC));
    const uint8_t *ssectorsbytes =
        const_cast<const uint8_t *>((const uint8_t *)W_CacheLumpNum(maplumpnum + ML_SSECTORS, PU_STATIC));
    const uint8_t *sectorsbytes =
        const_cast<const uint8_t *>((const uint8_t *)W_CacheLumpNum(maplumpnum + ML_SECTORS, PU_STATIC));

    levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum + ML_THINGS), *thingbytes);
    levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum + ML_LINEDEFS), *lindefbytes);
    levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum + ML_SIDEDEFS), *sidedefbytes);
    levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum + ML_VERTEXES), *vertexbytes);
    levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum + ML_SEGS), *segsbytes);
    levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum + ML_SSECTORS), *ssectorsbytes);
    levellumps.insert(levellumps.end(), W_LumpLength(maplumpnum + ML_SECTORS), *sectorsbytes);

    length = W_LumpLength(maplumpnum + ML_THINGS) + W_LumpLength(maplumpnum + ML_LINEDEFS) +
             W_LumpLength(maplumpnum + ML_SIDEDEFS) + W_LumpLength(maplumpnum + ML_VERTEXES) +
             W_LumpLength(maplumpnum + ML_SEGS) + W_LumpLength(maplumpnum + ML_SSECTORS) +
             W_LumpLength(maplumpnum + ML_SECTORS);

    std::string fingerprint = MD5SUM(levellumps.data(), length);

    memcpy(::level.level_fingerprint, fingerprint.data(), 16);
}
//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void P_GroupLines(void)
{
    line_t     **linebuffer;
    int32_t      i;
    int32_t      j;
    int32_t      total;
    line_t      *li;
    sector_t    *sector;
    DBoundingBox bbox;
    int32_t      block;

    // look up sector number for each subsector
    for (i = 0; i < numsubsectors; i++)
    {
        if (subsectors[i].firstline >= (uint32_t)numsegs)
            I_Error("subsector[%d].firstline exceeds numsegs (%u)", i, numsegs);
        subsectors[i].sector = segs[subsectors[i].firstline].sidedef->sector;
    }

    // count number of lines in each sector
    li    = lines;
    total = 0;
    for (i = 0; i < numlines; i++, li++)
    {
        total++;
        if (!li->frontsector && li->backsector)
        {
            // swap front and backsectors if a one-sided linedef
            // does not have a front sector
            li->frontsector = li->backsector;
            li->backsector  = NULL;
        }

        if (li->frontsector)
            li->frontsector->linecount++;

        if (li->backsector && li->backsector != li->frontsector)
        {
            li->backsector->linecount++;
            total++;
        }
    }

    // build line tables for each sector
    linebuffer = (line_t **)Z_Malloc(total * sizeof(line_t *), PU_LEVEL, 0);
    sector     = sectors;
    for (i = 0; i < numsectors; i++, sector++)
    {
        bbox.ClearBox();
        sector->lines = linebuffer;
        li            = lines;
        for (j = 0; j < numlines; j++, li++)
        {
            if (li->frontsector == sector || li->backsector == sector)
            {
                *linebuffer++ = li;
                bbox.AddToBox(li->v1->x, li->v1->y);
                bbox.AddToBox(li->v2->x, li->v2->y);
            }
        }
        if (linebuffer - sector->lines != sector->linecount)
            I_Error("P_GroupLines: miscounted");

        // set the soundorg to the middle of the bounding box
        sector->soundorg[0] = (bbox.Right() + bbox.Left()) / 2;
        sector->soundorg[1] = (bbox.Top() + bbox.Bottom()) / 2;

        // adjust bounding box to map blocks
        block                    = (bbox.Top() - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
        block                    = block >= bmapheight ? bmapheight - 1 : block;
        sector->blockbox[BOXTOP] = block;

        block                       = (bbox.Bottom() - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
        block                       = block < 0 ? 0 : block;
        sector->blockbox[BOXBOTTOM] = block;

        block                      = (bbox.Right() - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
        block                      = block >= bmapwidth ? bmapwidth - 1 : block;
        sector->blockbox[BOXRIGHT] = block;

        block                     = (bbox.Left() - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
        block                     = block < 0 ? 0 : block;
        sector->blockbox[BOXLEFT] = block;
    }
}

//
// P_RemoveSlimeTrails()
//
// killough 10/98
//
// Slime trails are inherent to Doom's coordinate system -- i.e. there is
// nothing that a node builder can do to prevent slime trails ALL of the time,
// because it's a product of the integer coodinate system, and just because
// two lines pass through exact integer coordinates, doesn't necessarily mean
// that they will intersect at integer coordinates. Thus we must allow for
// fractional coordinates if we are to be able to split segs with node lines,
// as a node builder must do when creating a BSP tree.
//
// A wad file does not allow fractional coordinates, so node builders are out
// of luck except that they can try to limit the number of splits (they might
// also be able to detect the degree of roundoff error and try to avoid splits
// with a high degree of roundoff error). But we can use fractional coordinates
// here, inside the engine. It's like the difference between square inches and
// square miles, in terms of granularity.
//
// For each vertex of every seg, check to see whether it's also a vertex of
// the linedef associated with the seg (i.e, it's an endpoint). If it's not
// an endpoint, and it wasn't already moved, move the vertex towards the
// linedef by projecting it using the law of cosines. Formula:
//
//      2        2                         2        2
//    dx  x0 + dy  x1 + dx dy (y0 - y1)  dy  y0 + dx  y1 + dx dy (x0 - x1)
//   {---------------------------------, ---------------------------------}
//                  2     2                            2     2
//                dx  + dy                           dx  + dy
//
// (x0,y0) is the vertex being moved, and (x1,y1)-(x1+dx,y1+dy) is the
// reference linedef.
//
// Segs corresponding to orthogonal linedefs (exactly vertical or horizontal
// linedefs), which comprise at least half of all linedefs in most wads, don't
// need to be considered, because they almost never contribute to slime trails
// (because then any roundoff error is parallel to the linedef, which doesn't
// cause slime). Skipping simple orthogonal lines lets the code finish quicker.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.
//
// Firelines (TM) is a Rezistered Trademark of MBF Productions
//

static void P_RemoveSlimeTrails()
{
    uint8_t *hit = (uint8_t *)Z_Malloc(numvertexes, PU_LEVEL, 0);
    memset(hit, 0, numvertexes * sizeof(uint8_t));

    for (int32_t i = 0; i < numsegs; i++)
    {
        const line_t *l = segs[i].linedef; // The parent linedef

        // We can ignore orthogonal lines
        if (l->slopetype != ST_VERTICAL && l->slopetype != ST_HORIZONTAL)
        {
            vertex_t *v = segs[i].v1;
            do
            {
                if (!hit[v - vertexes])           // If we haven't processed vertex
                {
                    hit[v - vertexes] = 1;        // Mark this vertex as processed
                    if (v != l->v1 && v != l->v2) // Exclude endpoints of linedefs
                    {
                        // Project the vertex back onto the parent linedef
                        int64_t dx2 = (l->dx >> FRACBITS) * (l->dx >> FRACBITS);
                        int64_t dy2 = (l->dy >> FRACBITS) * (l->dy >> FRACBITS);
                        int64_t dxy = (l->dx >> FRACBITS) * (l->dy >> FRACBITS);
                        int64_t s   = dx2 + dy2;
                        fixed_t x0 = v->x, y0 = v->y, x1 = l->v1->x, y1 = l->v1->y;
                        v->x = (fixed_t)((dx2 * x0 + dy2 * x1 + dxy * (y0 - y1)) / s);
                        v->y = (fixed_t)((dy2 * y0 + dx2 * y1 + dxy * (x0 - x1)) / s);
                    }
                } // Obsfucated C contest entry:   :)
            } while ((v != segs[i].v2) && (v = segs[i].v2));
        }
    }

    Z_Free(hit);
}

//
// [RH] P_LoadBehavior
//
void P_LoadBehavior(int32_t lumpnum)
{
    uint8_t *behavior = (uint8_t *)W_CacheLumpNum(lumpnum, PU_LEVEL);

    level.behavior = new FBehavior(behavior, lumpinfo[lumpnum].size);

    if (!level.behavior->IsGood())
    {
        delete level.behavior;
        level.behavior = NULL;
    }
}

//
// P_SetupLevel
//
extern polyblock_t **PolyBlockMap;

// Hash the sector tags across the sectors and linedefs.
static void P_InitTagLists(void)
{
    int32_t i;

    for (i = numsectors; --i >= 0;)                                            // Initially make all slots empty.
        sectors[i].firsttag = -1;
    for (i = numsectors; --i >= 0;)                                            // Proceed from last to first sector
    {                                                                          // so that lower sectors appear first
        int32_t j           = (uint32_t)sectors[i].tag % (uint32_t)numsectors; // Hash func
        sectors[i].nexttag  = sectors[j].firsttag;                             // Prepend sector to chain
        sectors[j].firsttag = i;
    }

    // killough 4/17/98: same thing, only for linedefs

    for (i = numlines; --i >= 0;)                                      // Initially make all slots empty.
        lines[i].firstid = -1;
    for (i = numlines; --i >= 0;)                                      // Proceed from last to first linedef
    {                                                                  // so that lower linedefs appear first
        int32_t j        = (uint32_t)lines[i].id % (uint32_t)numlines; // Hash func
        lines[i].nextid  = lines[j].firstid;                           // Prepend linedef to chain
        lines[j].firstid = i;
    }
}

// [RH] position indicates the start spot to spawn at
void P_SetupLevel(const char *lumpname, int32_t position)
{
    size_t lumpnum;

    level.total_monsters = level.respawned_monsters = level.total_items = level.total_secrets = level.killed_monsters =
        level.found_items = level.found_secrets = wminfo.maxfrags = 0;
    ArrayInit(level.level_fingerprint, 0);
    wminfo.partime = 180;

    if (!savegamerestore)
    {
        for (Players::iterator it = players.begin(); it != players.end(); ++it)
        {
            it->killcount = it->secretcount = it->itemcount = 0;
        }
    }

    // To use the correct nodes for

    // Initial height of PointOfView will be set by player think.
    consoleplayer().viewz = 1;

    // Make sure all sounds are stopped before Z_FreeTags.
    S_Start();

    // [RH] Clear all ThingID hash chains.
    AActor::ClearTIDHashes();

    PolyBlockMap = NULL;

    // [AM] So shootthing isn't a wild pointer on map swtich.
    shootthing = NULL;

    DThinker::DestroyAllThinkers();
    Z_FreeTags(PU_LEVEL, PU_LEVELMAX);
    g_ValidLevel = false; // [AM] False until the level is loaded.
    // [RH] Z_FreeTags frees all the custom colormaps
    NormalLight.next = nullptr;

    // [AM] Every new level starts with fresh netids.
    P_ClearAllNetIds();

    // find map num
    lumpnum = W_GetNumForName(lumpname);

    // [RH] Check if this map is Hexen-style.
    //		LINEDEFS and THINGS need to be handled accordingly.
    //		If it is, we also need to distinguish between projectile cross and hit
    HasBehavior = W_CheckLumpName(lumpnum + ML_BEHAVIOR, "BEHAVIOR");
    // oldshootactivation = !HasBehavior;

    // note: most of this ordering is important

    // [RH] Load in the BEHAVIOR lump
    if (level.behavior != NULL)
    {
        delete level.behavior;
        level.behavior = NULL;
    }

    // [Blair] Create map fingerprint
    P_GenerateUniqueMapFingerPrint(lumpnum);

    if (HasBehavior)
    {
        P_LoadBehavior(lumpnum + ML_BEHAVIOR);
        map_format.P_ApplyZDoomMapFormat();
    }
    else
    {
        map_format.P_ApplyDefaultMapFormat();
    }

    level.time = 0;

    P_LoadVertexes(lumpnum + ML_VERTEXES);
    P_LoadSectors(lumpnum + ML_SECTORS);
    P_LoadSideDefs(lumpnum + ML_SIDEDEFS);
    if (!HasBehavior)
        P_LoadLineDefs(lumpnum + ML_LINEDEFS);
    else
        P_LoadLineDefs2(lumpnum + ML_LINEDEFS); // [RH] Load Hexen-style linedefs
    P_LoadSideDefs2(lumpnum + ML_SIDEDEFS);
    P_FinishLoadingLineDefs();
    P_LoadBlockMap(lumpnum + ML_BLOCKMAP);

    if (!P_LoadXNOD(lumpnum + ML_NODES))
    {
        P_LoadSubsectors(lumpnum + ML_SSECTORS);
        P_LoadNodes(lumpnum + ML_NODES);
        P_LoadSegs(lumpnum + ML_SEGS);
    }

    rejectmatrix = (uint8_t *)W_CacheLumpNum(lumpnum + ML_REJECT, PU_LEVEL);
    {
        // [SL] 2011-07-01 - Check to see if the reject table is of the proper size
        // If it's too short, the reject table should be ignored when
        // calling P_CheckSight
        if (W_LumpLength(lumpnum + ML_REJECT) < ((uint32_t)ceil((float)(numsectors * numsectors / 8))))
        {
            DPrintf("Reject matrix is not valid and will be ignored.\n");
            rejectempty = true;
        }
    }
    P_GroupLines();

    // [SL] don't move seg vertices if compatibility is cruical
    P_RemoveSlimeTrails();

    P_SetupSlopes();

    po_NumPolyobjs = 0;

    P_InitTagLists(); // killough 1/30/98: Create xref tables for tags

    if (!HasBehavior)
        P_LoadThings(lumpnum + ML_THINGS);
    else
        P_LoadThings2(lumpnum + ML_THINGS, position); // [RH] Load Hexen-style things

    if (!HasBehavior)
        P_TranslateTeleportThings();                  // [RH] Assign teleport destination TIDs

    PO_Init();

    if (serverside)
    {
        for (Players::iterator it = players.begin(); it != players.end(); ++it)
        {
            SV_PreservePlayer(*it);

            if (it->ingame())
            {
                // if deathmatch, randomly spawn the active players
                // denis - this function checks for deathmatch internally
                G_DeathMatchSpawnPlayer(*it);
            }
        }
    }

    // clear special respawning que
    iquehead = iquetail = 0;

    // killough 3/26/98: Spawn icon landings:
    P_SpawnBrainTargets();

    // set up world state
    P_SetupWorldState();

    // build subsector connect matrix
    //	UNUSED P_ConnectSubsectors ();

    // [AM] Level is now safely loaded.
    g_ValidLevel = true;
}

//
// P_Init
//
void P_Init(void)
{
    P_InitSwitchList();
    InitTeamInfo();
}

CVAR_FUNC_IMPL(sv_intermissionlimit)
{
    if (G_IsCoopGame() && var < 10)
    {
        var.Set(10.0); // Force to 10 seconds minimum
    }
    else if (var < 1)
    {
        var.RestoreDefault();
    }

    level.inttimeleft = var;
}

static void P_SetupLevelFloorPlane(sector_t *sector)
{
    if (!sector)
        return;

    sector->floorplane.a = sector->floorplane.b = 0;
    sector->floorplane.c = sector->floorplane.invc = FRACUNIT;
    sector->floorplane.d                           = -sector->floorheight;
    sector->floorplane.texx = sector->floorplane.texy = 0;
    sector->floorplane.sector                         = sector;
}

static void P_SetupLevelCeilingPlane(sector_t *sector)
{
    if (!sector)
        return;

    sector->ceilingplane.a = sector->ceilingplane.b = 0;
    sector->ceilingplane.c = sector->ceilingplane.invc = -FRACUNIT;
    sector->ceilingplane.d                             = sector->ceilingheight;
    sector->ceilingplane.texx = sector->ceilingplane.texy = 0;
    sector->ceilingplane.sector                           = sector;
}

//
// P_SetupPlane()
//
// Takes a line with the special property Plane_Align and its facing sector
// and calculates the planar equation for the slope formed by the floor or
// ceiling of this sector.  The equation coefficients are stored in a plane_t
// structure and saved either to the sector's ceilingplan or floorplane.
//
void P_SetupPlane(sector_t *sec, line_t *line, bool floor)
{
    if (!sec || !line || !line->backsector)
        return;

    // Find the vertex comprising the sector that is farthest from the
    // slope's reference line

    int32_t   bestdist = 0;
    line_t  **probe    = sec->lines;
    vertex_t *refvert  = (*sec->lines)->v1;

    for (int32_t i = sec->linecount * 2; i > 0; i--)
    {
        int32_t   dist;
        vertex_t *vert;

        // Do calculations with only the upper bits, because the lower ones
        // are all zero, and we would overflow for a lot of distances if we
        // kept them around.

        if (i & 1)
            vert = (*probe++)->v2;
        else
            vert = (*probe)->v1;
        dist = abs(((line->v1->y - vert->y) >> FRACBITS) * (line->dx >> FRACBITS) -
                   ((line->v1->x - vert->x) >> FRACBITS) * (line->dy >> FRACBITS));

        if (dist > bestdist)
        {
            bestdist = dist;
            refvert  = vert;
        }
    }

    const sector_t *refsec     = line->frontsector == sec ? line->backsector : line->frontsector;
    plane_t        *srcplane   = floor ? &sec->floorplane : &sec->ceilingplane;
    fixed_t         srcheight  = floor ? sec->floorheight : sec->ceilingheight;
    fixed_t         destheight = floor ? refsec->floorheight : refsec->ceilingheight;

    v3float_t p, v1, v2, cross;
    M_SetVec3f(&p, line->v1->x, line->v1->y, destheight);
    M_SetVec3f(&v1, line->dx, line->dy, 0);
    M_SetVec3f(&v2, refvert->x - line->v1->x, refvert->y - line->v1->y, srcheight - destheight);

    M_CrossProductVec3f(&cross, &v1, &v2);
    M_NormalizeVec3f(&cross, &cross);

    // Fix backward normals
    if ((cross.z < 0 && floor == true) || (cross.z > 0 && floor == false))
    {
        cross.x = -cross.x;
        cross.y = -cross.y;
        cross.z = -cross.z;
    }

    srcplane->a    = FLOAT2FIXED(cross.x);
    srcplane->b    = FLOAT2FIXED(cross.y);
    srcplane->c    = FLOAT2FIXED(cross.z);
    srcplane->invc = FLOAT2FIXED(1.f / cross.z);
    srcplane->d =
        -FixedMul(srcplane->a, line->v1->x) - FixedMul(srcplane->b, line->v1->y) - FixedMul(srcplane->c, destheight);
    srcplane->texx = refvert->x;
    srcplane->texy = refvert->y;
}

static void P_SetupSlopes()
{
    for (int32_t i = 0; i < numlines; i++)
    {
        line_t *line = &lines[i];

        int16_t spec = line->special;

        if ((map_format.getZDoom() && line->special == Plane_Align) || (line->special >= 340 && line->special <= 347))
        {
            line->special = 0;
            line->id      = line->args[2];

            // Floor plane?
            int32_t align_side = line->args[0] & 3;
            if (align_side == 1)
                P_SetupPlane(line->frontsector, line, true);
            else if (align_side == 2)
                P_SetupPlane(line->backsector, line, true);

            // Ceiling plane?
            align_side = line->args[1] & 3;
            if (align_side == 0)
                align_side = (line->args[0] >> 2) & 3;

            if (align_side == 1)
                P_SetupPlane(line->frontsector, line, false);
            else if (align_side == 2)
                P_SetupPlane(line->backsector, line, false);
        }
    }
}

VERSION_CONTROL(p_setup_cpp, "$Id: cb76b8e6a9fff8e6ec132bbd8132ae61f1244a67 $")
