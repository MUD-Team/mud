//----------------------------------------------------------------------------
//  EDGE Level Loading/Setup Code
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2024 The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "p_setup.h"

#include <map>

#include "AlmostEquals.h"
#include "ddf_colormap.h"
#include "ddf_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_main.h"
#include "epi_ename.h"
#include "epi_endian.h"
#include "epi_filesystem.h"
#include "epi_lexer.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "g_game.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_math.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_local.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_sky.h"
#include "s_music.h"
#include "s_sound.h"
#include "sokol_color.h"
#include "sv_main.h"
#include "w_files.h"


#define EDGE_SEG_INVALID       ((Seg *)-3)
#define EDGE_SUBSECTOR_INVALID ((Subsector *)-3)

extern unsigned int root_node;

static bool level_active = false;

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//

int                 total_level_vertexes;
Vertex             *level_vertexes = nullptr;
static Vertex      *level_gl_vertexes;
int                 total_level_segs;
Seg                *level_segs;
int                 total_level_sectors;
Sector             *level_sectors;
int                 total_level_subsectors;
Subsector          *level_subsectors;
int                 total_level_nodes;
BspNode            *level_nodes;
int                 total_level_lines;
Line               *level_lines;
float              *level_line_alphas; // UDMF processing
int                 total_level_sides;
Side               *level_sides;
static int          total_level_vertical_gaps;
static VerticalGap *level_vertical_gaps;

VertexSectorList *level_vertex_sector_lists;

static Line **level_line_buffer = nullptr;

// bbox used
static float dummy_bounding_box[4];

int total_map_things;

static std::string udmf_string;
static std::string node_file;

// a place to store sidedef numbers of the loaded linedefs.
// There is two values for every line: side0 and side1.
static int *temp_line_sides;

static void SegCommonStuff(Seg *seg, int linedef_in)
{
    seg->front_sector = seg->back_sector = nullptr;

    if (linedef_in == -1)
    {
        seg->miniseg = true;
    }
    else
    {
        if (linedef_in >= total_level_lines) // sanity check
            FatalError("Bad GWA file: seg #%d has invalid linedef.\n", (int)(seg - level_segs));

        seg->miniseg = false;
        seg->linedef = &level_lines[linedef_in];

        float sx = seg->side ? seg->linedef->vertex_2->x : seg->linedef->vertex_1->x;
        float sy = seg->side ? seg->linedef->vertex_2->y : seg->linedef->vertex_1->y;

        seg->offset = PointToDistance(sx, sy, seg->vertex_1->x, seg->vertex_1->y);

        seg->sidedef = seg->linedef->side[seg->side];

        if (!seg->sidedef)
            FatalError("Bad GWA file: missing side for seg #%d\n", (int)(seg - level_segs));

        seg->front_sector = seg->sidedef->sector;

        if (seg->linedef->flags & kLineFlagTwoSided)
        {
            Side *other = seg->linedef->side[seg->side ^ 1];

            if (other)
                seg->back_sector = other->sector;
        }
    }
}

//
// GroupSectorTags
//
// Called during P_LoadSectors to set the tag_next & tag_previous fields of
// each sector_t, which keep all sectors with the same tag in a linked
// list for faster handling.
//
// -AJA- 1999/07/29: written.
//
static void GroupSectorTags(Sector *dest, Sector *seclist, int numsecs)
{
    // NOTE: `numsecs' does not include the current sector.

    dest->tag_next = dest->tag_previous = nullptr;

    for (; numsecs > 0; numsecs--)
    {
        Sector *src = &seclist[numsecs - 1];

        if (src->tag == dest->tag)
        {
            src->tag_next      = dest;
            dest->tag_previous = src;
            return;
        }
    }
}

static void SetupRootNode(void)
{
    if (total_level_nodes > 0)
    {
        root_node = total_level_nodes - 1;
    }
    else
    {
        root_node = kLeafSubsector | 0;

        // compute bbox for the single subsector
        BoundingBoxClear(dummy_bounding_box);

        int  i;
        Seg *seg;

        for (i = 0, seg = level_segs; i < total_level_segs; i++, seg++)
        {
            BoundingBoxAddPoint(dummy_bounding_box, seg->vertex_1->x, seg->vertex_1->y);
            BoundingBoxAddPoint(dummy_bounding_box, seg->vertex_2->x, seg->vertex_2->y);
        }
    }
}

static std::map<int, int> unknown_thing_map;

static void UnknownThingWarning(int type, float x, float y)
{
    int count = 0;

    if (unknown_thing_map.find(type) != unknown_thing_map.end())
        count = unknown_thing_map[type];

    if (count < 2)
        LogWarning("Unknown thing type %i at (%1.0f, %1.0f)\n", type, x, y);
    else if (count == 2)
        LogWarning("More unknown things of type %i found...\n", type);

    unknown_thing_map[type] = count + 1;
}

static MapObject *SpawnMapThing(const MapObjectDefinition *info, float x, float y, float z, Sector *sec, BAMAngle angle,
                                int options, int tag)
{
    SpawnPoint point;

    point.x              = x;
    point.y              = y;
    point.z              = z;
    point.angle          = angle;
    point.vertical_angle = 0;
    point.info           = info;
    point.flags          = 0;
    point.tag            = tag;

    // -KM- 1999/01/31 Use playernum property.
    // count deathmatch start positions
    if (info->playernum_ < 0)
    {
        AddDeathmatchStart(point);
        return nullptr;
    }

    // check for players specially -jc-
    if (info->playernum_ > 0)
    {
        // -AJA- 2009/10/07: Hub support
        if (sec->properties.special && sec->properties.special->hub_)
        {
            if (sec->tag <= 0)
                LogWarning("HUB_START in sector without tag @ (%1.0f %1.0f)\n", x, y);

            point.tag = sec->tag;

            AddHubStart(point);
            return nullptr;
        }

        SpawnPoint *prev = FindCoopPlayer(info->playernum_);

        if (!prev)
            AddCoopStart(point);

        return nullptr;
    }

    // check for apropriate skill level
    // -ES- 1999/04/13 Implemented Kester's Bugfix.
    // -AJA- 1999/10/21: Reworked again.
    if (InSinglePlayerMatch() && (options & kThingNotSinglePlayer))
        return nullptr;

    // -AJA- 1999/09/22: Boom compatibility flags.
    if (InCooperativeMatch() && (options & kThingNotCooperative))
        return nullptr;

    if (InDeathmatch() && (options & kThingNotDeathmatch))
        return nullptr;

    int bit;

    if (game_skill == kSkillBaby)
        bit = 1;
    else if (game_skill == kSkillNightmare)
        bit = 4;
    else
        bit = 1 << (game_skill - 1);

    if ((options & bit) == 0)
        return nullptr;

    // don't spawn keycards in deathmatch
    if (InDeathmatch() && (info->flags_ & kMapObjectFlagNotDeathmatch))
        return nullptr;

    // don't spawn any monsters if -nomonsters
    if (level_flags.no_monsters && (info->extended_flags_ & kExtendedFlagMonster))
        return nullptr;

    // spawn it now !
    // Use MobjCreateObject -ACB- 1998/08/06
    MapObject *mo = CreateMapObject(x, y, z, info);

    mo->angle_      = angle;
    mo->spawnpoint_ = point;

    if (mo->state_ && mo->state_->tics > 1)
        mo->tics_ = 1 + (RandomByteDeterministic() % mo->state_->tics);

    if (options & kThingAmbush)
    {
        mo->flags_ |= kMapObjectFlagAmbush;
        mo->spawnpoint_.flags |= kMapObjectFlagAmbush;
    }

    // -AJA- 2000/09/22: MBF compatibility flag
    if (options & kThingFriend)
    {
        mo->side_ = 1; //~0;
        mo->hyper_flags_ |= kHyperFlagUltraLoyal;
    }
    // Lobo 2022: added tagged mobj support ;)
    if (tag > 0)
        mo->tag_ = tag;

    return mo;
}

static inline void ComputeLinedefData(Line *ld, int side0, int side1)
{
    Vertex *v1 = ld->vertex_1;
    Vertex *v2 = ld->vertex_2;

    ld->delta_x = v2->x - v1->x;
    ld->delta_y = v2->y - v1->y;

    if (AlmostEquals(ld->delta_x, 0.0f))
        ld->slope_type = kLineClipVertical;
    else if (AlmostEquals(ld->delta_y, 0.0f))
        ld->slope_type = kLineClipHorizontal;
    else if (ld->delta_y / ld->delta_x > 0)
        ld->slope_type = kLineClipPositive;
    else
        ld->slope_type = kLineClipNegative;

    ld->length = PointToDistance(0, 0, ld->delta_x, ld->delta_y);

    if (v1->x < v2->x)
    {
        ld->bounding_box[kBoundingBoxLeft]  = v1->x;
        ld->bounding_box[kBoundingBoxRight] = v2->x;
    }
    else
    {
        ld->bounding_box[kBoundingBoxLeft]  = v2->x;
        ld->bounding_box[kBoundingBoxRight] = v1->x;
    }

    if (v1->y < v2->y)
    {
        ld->bounding_box[kBoundingBoxBottom] = v1->y;
        ld->bounding_box[kBoundingBoxTop]    = v2->y;
    }
    else
    {
        ld->bounding_box[kBoundingBoxBottom] = v2->y;
        ld->bounding_box[kBoundingBoxTop]    = v1->y;
    }

    // handle missing RIGHT sidedef (idea taken from MBF)
    if (side0 == -1)
    {
        LogWarning("Bad WAD: level %s linedef #%d is missing RIGHT side\n", current_map->name_.c_str(),
                   (int)(ld - level_lines));
        side0 = 0;
    }

    if ((ld->flags & kLineFlagTwoSided) && ((side0 == -1) || (side1 == -1)))
    {
        LogWarning("Bad WAD: level %s has linedef #%d marked TWOSIDED, "
                   "but it has only one side.\n",
                   current_map->name_.c_str(), (int)(ld - level_lines));

        ld->flags &= ~kLineFlagTwoSided;
    }

    temp_line_sides[(ld - level_lines) * 2 + 0] = side0;
    temp_line_sides[(ld - level_lines) * 2 + 1] = side1;

    total_level_sides += (side1 == -1) ? 1 : 2;
}

static Sector *DetermineSubsectorSector(Subsector *ss, int pass)
{
    const Seg *seg;

    for (seg = ss->segs; seg != nullptr; seg = seg->subsector_next)
    {
        if (seg->miniseg)
            continue;

        // ignore self-referencing linedefs
        if (seg->front_sector == seg->back_sector)
            continue;

        return seg->front_sector;
    }

    for (seg = ss->segs; seg != nullptr; seg = seg->subsector_next)
    {
        if (seg->partner == nullptr)
            continue;

        // only do this for self-referencing linedefs if the original sector
        // isn't tagged, otherwise save it for the next pass
        if (seg->front_sector == seg->back_sector && seg->front_sector && seg->front_sector->tag == 0)
            return seg->front_sector;

        if (seg->front_sector != seg->back_sector && seg->partner->front_subsector->sector != nullptr)
            return seg->partner->front_subsector->sector;
    }

    if (pass == 1)
    {
        for (seg = ss->segs; seg != nullptr; seg = seg->subsector_next)
        {
            if (!seg->miniseg)
                return seg->front_sector;
        }
    }

    if (pass == 2)
        return &level_sectors[0];

    return nullptr;
}

static bool AssignSubsectorsPass(int pass)
{
    // pass 0 : ignore self-ref lines.
    // pass 1 : use them.
    // pass 2 : handle extreme brokenness.
    //
    // returns true if progress was made.

    bool progress   = false;

    for (int i = 0; i < total_level_subsectors; i++)
    {
        Subsector *ss = &level_subsectors[i];

        if (ss->sector == nullptr)
        {
            ss->sector = DetermineSubsectorSector(ss, pass);

            if (ss->sector != nullptr)
            {
                progress = true;

                // link subsector into parent sector's list.
                // order is not important, so add it to the head of the list.
                ss->sector_next        = ss->sector->subsectors;
                ss->sector->subsectors = ss;
            }
        }
    }

    return progress;
}

static void AssignSubsectorsToSectors()
{
    // AJA 2022: this attempts to improve handling of self-referencing lines
    //           (i.e. ones with the same sector on both sides).  Subsectors
    //           touching such lines should NOT be assigned to that line's
    //           sector, but rather to the "outer" sector.

    while (AssignSubsectorsPass(0))
    {
    }

    while (AssignSubsectorsPass(1))
    {
    }

    // the above *should* handle everything, so this pass is only needed
    // for extremely broken nodes or maps.
    AssignSubsectorsPass(2);
}

// Adapted from EDGE 2.x's ZNode loading routine; only handles XGL3 as that
// is all our built-in AJBSP produces now
static void LoadXGL3Nodes()
{
    int                  i, xglen = 0;
    uint8_t             *xgldata = nullptr;
    uint8_t             *td = nullptr;

    LogDebug("LoadXGL3Nodes:\n");

    epi::File *xgl_file = epi::FileOpen(node_file, epi::kFileAccessRead);

    if (!xgl_file)
        FatalError("LoadXGL3Nodes: Couldn't load lump\n");

    xgldata = xgl_file->LoadIntoMemory();
    xglen = xgl_file->GetLength();

    delete xgl_file;

    if (xglen < 12)
    {
        delete[] xgldata;
        FatalError("LoadXGL3Nodes: Lump too short\n");
    }

    if (!memcmp(xgldata, "XGL3", 4))
        LogDebug(" AJBSP uncompressed GL nodes v3\n");
    else
    {
        static char xgltemp[6];
        epi::CStringCopyMax(xgltemp, (char *)xgldata, 4);
        delete[] xgldata;
        FatalError("LoadXGL3Nodes: Unrecognized node type %s\n", xgltemp);
    }

    td = &xgldata[4];

    // after signature, 1st u32 is number of original vertexes - should be <=
    // total_level_vertexes
    int oVerts = epi::UnalignedLittleEndianU32(td);
    td += 4;
    if (oVerts > total_level_vertexes)
    {
        delete[] xgldata;
        FatalError("LoadXGL3Nodes: Vertex/Node mismatch\n");
    }

    // 2nd u32 is the number of extra vertexes added by ajbsp
    int nVerts = epi::UnalignedLittleEndianU32(td);
    td += 4;
    LogDebug("LoadXGL3Nodes: Orig Verts = %d, New Verts = %d, Map Verts = %d\n", oVerts, nVerts, total_level_vertexes);

    level_gl_vertexes = new Vertex[nVerts];

    // fill in new vertexes
    Vertex *vv = level_gl_vertexes;
    for (i = 0; i < nVerts; i++, vv++)
    {
        // convert signed 16.16 fixed point to float
        vv->x = (float)epi::UnalignedLittleEndianS32(td) / 65536.0f;
        td += 4;
        vv->y = (float)epi::UnalignedLittleEndianS32(td) / 65536.0f;
        td += 4;
        vv->z = -40000.0f;
        vv->w = 40000.0f;
    }

    // new vertexes is followed by the subsectors
    total_level_subsectors = epi::UnalignedLittleEndianS32(td);
    td += 4;
    if (total_level_subsectors <= 0)
    {
        delete[] xgldata;
        FatalError("LoadXGL3Nodes: No subsectors\n");
    }
    LogDebug("LoadXGL3Nodes: Num SSECTORS = %d\n", total_level_subsectors);

    level_subsectors = new Subsector[total_level_subsectors];
    EPI_CLEAR_MEMORY(level_subsectors, Subsector, total_level_subsectors);

    int *ss_temp = new int[total_level_subsectors];
    int  xglSegs = 0;
    for (i = 0; i < total_level_subsectors; i++)
    {
        int countsegs = epi::UnalignedLittleEndianS32(td);
        td += 4;
        ss_temp[i] = countsegs;
        xglSegs += countsegs;
    }

    // subsectors are followed by the segs
    total_level_segs = epi::UnalignedLittleEndianS32(td);
    td += 4;
    if (total_level_segs != xglSegs)
    {
        delete[] xgldata;
        FatalError("LoadXGL3Nodes: Incorrect number of segs in nodes\n");
    }
    LogDebug("LoadXGL3Nodes: Num SEGS = %d\n", total_level_segs);

    level_segs = new Seg[total_level_segs];
    EPI_CLEAR_MEMORY(level_segs, Seg, total_level_segs);
    Seg *seg = level_segs;

    for (i = 0; i < total_level_segs; i++, seg++)
    {
        unsigned int v1num;
        int          slinedef, partner, side;

        v1num = epi::UnalignedLittleEndianU32(td);
        td += 4;
        partner = epi::UnalignedLittleEndianS32(td);
        td += 4;
        slinedef = epi::UnalignedLittleEndianS32(td);
        td += 4;
        side = (int)(*td);
        td += 1;

        if (v1num < (uint32_t)oVerts)
            seg->vertex_1 = &level_vertexes[v1num];
        else
            seg->vertex_1 = &level_gl_vertexes[v1num - oVerts];

        seg->side = side ? 1 : 0;

        if (partner == -1)
            seg->partner = nullptr;
        else
        {
            EPI_ASSERT(partner < total_level_segs); // sanity check
            seg->partner = &level_segs[partner];
        }

        SegCommonStuff(seg, slinedef);

        // The following fields are filled out elsewhere:
        //     sub_next, front_sub, back_sub, frontsector, backsector.

        seg->subsector_next  = EDGE_SEG_INVALID;
        seg->front_subsector = seg->back_subsector = EDGE_SUBSECTOR_INVALID;
    }

    LogDebug("LoadXGL3Nodes: Post-process subsectors\n");
    // go back and fill in subsectors
    Subsector *ss = level_subsectors;
    xglSegs       = 0;
    for (i = 0; i < total_level_subsectors; i++, ss++)
    {
        int countsegs = ss_temp[i];
        int firstseg  = xglSegs;
        xglSegs += countsegs;

        // go back and fill in v2 from v1 of next seg and do calcs that needed
        // both
        seg = &level_segs[firstseg];
        for (int j = 0; j < countsegs; j++, seg++)
        {
            seg->vertex_2 =
                j == (countsegs - 1) ? level_segs[firstseg].vertex_1 : level_segs[firstseg + j + 1].vertex_1;

            seg->angle = PointToAngle(seg->vertex_1->x, seg->vertex_1->y, seg->vertex_2->x, seg->vertex_2->y);

            seg->length =
                PointToDistance(seg->vertex_1->x, seg->vertex_1->y, seg->vertex_2->x, seg->vertex_2->y);
        }

        // -AJA- 1999/09/23: New linked list for the segs of a subsector
        //       (part of true bsp rendering).
        Seg **prevptr = &ss->segs;

        if (countsegs == 0)
            FatalError("LoadXGL3Nodes: level %s has invalid SSECTORS.\n", current_map->name_.c_str());

        ss->sector     = nullptr;
        ss->thing_list = nullptr;

        // this is updated when the nodes are loaded
        ss->bounding_box = dummy_bounding_box;

        for (int j = 0; j < countsegs; j++)
        {
            Seg *cur = &level_segs[firstseg + j];

            *prevptr = cur;
            prevptr  = &cur->subsector_next;

            cur->front_subsector = ss;
            cur->back_subsector  = nullptr;

            // LogDebug("  ssec = %d, seg = %d\n", i, firstseg + j);
        }
        // LogDebug("LoadZNodes: ssec = %d, fseg = %d, cseg = %d\n", i,
        // firstseg, countsegs);

        *prevptr = nullptr;
    }
    delete[] ss_temp; // CA 9.30.18: allocated with new but released using
                      // delete, added [] between delete and ss_temp

    LogDebug("LoadXGL3Nodes: Read GL nodes\n");
    // finally, read the nodes
    // NOTE: no nodes is okay (a basic single sector map). -AJA-
    total_level_nodes = epi::UnalignedLittleEndianU32(td);
    td += 4;
    LogDebug("LoadXGL3Nodes: Num nodes = %d\n", total_level_nodes);

    level_nodes = new BspNode[total_level_nodes + 1];
    EPI_CLEAR_MEMORY(level_nodes, BspNode, total_level_nodes);
    BspNode *nd = level_nodes;

    for (i = 0; i < total_level_nodes; i++, nd++)
    {
        nd->divider.x = (float)epi::UnalignedLittleEndianS32(td) / 65536.0f;
        td += 4;
        nd->divider.y = (float)epi::UnalignedLittleEndianS32(td) / 65536.0f;
        td += 4;
        nd->divider.delta_x = (float)epi::UnalignedLittleEndianS32(td) / 65536.0f;
        td += 4;
        nd->divider.delta_y = (float)epi::UnalignedLittleEndianS32(td) / 65536.0f;
        td += 4;

        nd->divider_length = PointToDistance(0, 0, nd->divider.delta_x, nd->divider.delta_y);

        for (int j = 0; j < 2; j++)
            for (int k = 0; k < 4; k++)
            {
                nd->bounding_boxes[j][k] = (float)epi::UnalignedLittleEndianS16(td);
                td += 2;
            }

        for (int j = 0; j < 2; j++)
        {
            nd->children[j] = epi::UnalignedLittleEndianU32(td);
            td += 4;

            // update bbox pointers in subsector
            if (nd->children[j] & kLeafSubsector)
            {
                Subsector *sss    = level_subsectors + (nd->children[j] & ~kLeafSubsector);
                sss->bounding_box = &nd->bounding_boxes[j][0];
            }
        }
    }

    AssignSubsectorsToSectors();

    LogDebug("LoadXGL3Nodes: Setup root node\n");
    SetupRootNode();

    LogDebug("LoadXGL3Nodes: Finished\n");
    delete[] xgldata;
}

static void LoadUDMFVertexes()
{
    epi::Lexer lex(udmf_string);

    LogDebug("LoadUDMFVertexes: parsing TEXTMAP\n");
    int cur_vertex = 0;
    int min_x = 0;
    int min_y = 0;
    int max_x = 0;
    int max_y = 0;

    for (;;)
    {
        std::string    section;
        epi::TokenKind tok = lex.Next(section);

        if (tok == epi::kTokenEOF)
            break;

        if (tok != epi::kTokenIdentifier)
            FatalError("Malformed TEXTMAP lump.\n");

        // check namespace
        if (lex.Match("="))
        {
            lex.Next(section);
            if (!lex.Match(";"))
                FatalError("Malformed TEXTMAP lump: missing ';'\n");
            continue;
        }

        if (!lex.Match("{"))
            FatalError("Malformed TEXTMAP lump: missing '{'\n");

        if (section == "vertex")
        {
            float x = 0.0f, y = 0.0f;
            float zf = -40000.0f, zc = 40000.0f;
            for (;;)
            {
                if (lex.Match("}"))
                    break;

                std::string    key;
                std::string    value;
                epi::TokenKind block_tok = lex.Next(key);

                if (block_tok == epi::kTokenEOF)
                    FatalError("Malformed TEXTMAP lump: unclosed block\n");

                if (block_tok != epi::kTokenIdentifier)
                    FatalError("Malformed TEXTMAP lump: missing key\n");

                if (!lex.Match("="))
                    FatalError("Malformed TEXTMAP lump: missing '='\n");

                block_tok = lex.Next(value);

                if (block_tok == epi::kTokenEOF || block_tok == epi::kTokenError || value == "}")
                    FatalError("Malformed TEXTMAP lump: missing value\n");

                if (!lex.Match(";"))
                    FatalError("Malformed TEXTMAP lump: missing ';'\n");

                epi::EName key_ename(key, true);

                switch (key_ename.GetIndex())
                {
                case epi::kENameX:
                    x = epi::LexDouble(value);
                    min_x = GLM_MIN((int)x, min_x);
                    max_x = GLM_MAX((int)x, max_x);
                    break;
                case epi::kENameY:
                    y = epi::LexDouble(value);
                    min_y = GLM_MIN((int)y, min_y);
                    max_y = GLM_MAX((int)y, max_y);
                    break;
                case epi::kENameZfloor:
                    zf = epi::LexDouble(value);
                    break;
                case epi::kENameZceiling:
                    zc = epi::LexDouble(value);
                    break;
                default:
                    break;
                }
            }
            level_vertexes[cur_vertex] = {{x, y, zf, zc}};
            cur_vertex++;
        }
        else // consume other blocks
        {
            for (;;)
            {
                tok = lex.Next(section);
                if (lex.Match("}") || tok == epi::kTokenEOF)
                    break;
            }
        }
    }
    EPI_ASSERT(cur_vertex == total_level_vertexes);

    GenerateBlockmap(min_x, min_y, max_x, max_y);

    CreateThingBlockmap();  

    LogDebug("LoadUDMFVertexes: finished parsing TEXTMAP\n");
}

static void LoadUDMFSectors()
{
    epi::Lexer lex(udmf_string);

    LogDebug("LoadUDMFSectors: parsing TEXTMAP\n");
    int cur_sector = 0;

    for (;;)
    {
        std::string    section;
        epi::TokenKind tok = lex.Next(section);

        if (tok == epi::kTokenEOF)
            break;

        if (tok != epi::kTokenIdentifier)
            FatalError("Malformed TEXTMAP lump.\n");

        // check namespace
        if (lex.Match("="))
        {
            lex.Next(section);
            if (!lex.Match(";"))
                FatalError("Malformed TEXTMAP lump: missing ';'\n");
            continue;
        }

        if (!lex.Match("{"))
            FatalError("Malformed TEXTMAP lump: missing '{'\n");

        if (section == "sector")
        {
            int       cz = 0, fz = 0, liquid_z = 0;
            float     fx = 0.0f, fy = 0.0f, cx = 0.0f, cy = 0.0f;
            float     fx_sc = 1.0f, fy_sc = 1.0f, cx_sc = 1.0f, cy_sc = 1.0f;
            float     falph = 1.0f, calph = 1.0f;
            float     rf = 0.0f, rc = 0.0f;
            float     gravfactor = 1.0f;
            int       light = 160, liquid_light = 144, type = 0, tag = 0;
            float     liquid_trans = 0.5f;
            //RGBAColor fog_color   = SG_BLACK_RGBA32;
            RGBAColor light_color = SG_WHITE_RGBA32;
            RGBAColor liquid_color = SG_STEEL_BLUE_RGBA32;
            //int       fog_density = 0;
            char      floor_tex[10];
            char      ceil_tex[10];
            char      liquid_tex[10];
            strcpy(floor_tex, "-");
            strcpy(ceil_tex, "-");
            strcpy(liquid_tex, "-");
            for (;;)
            {
                if (lex.Match("}"))
                    break;

                std::string    key;
                std::string    value;
                epi::TokenKind block_tok = lex.Next(key);

                if (block_tok == epi::kTokenEOF)
                    FatalError("Malformed TEXTMAP lump: unclosed block\n");

                if (block_tok != epi::kTokenIdentifier)
                    FatalError("Malformed TEXTMAP lump: missing key\n");

                if (!lex.Match("="))
                    FatalError("Malformed TEXTMAP lump: missing '='\n");

                block_tok = lex.Next(value);

                if (block_tok == epi::kTokenEOF || block_tok == epi::kTokenError || value == "}")
                    FatalError("Malformed TEXTMAP lump: missing value\n");

                if (!lex.Match(";"))
                    FatalError("Malformed TEXTMAP lump: missing ';'\n");

                epi::EName key_ename(key, true);

                switch (key_ename.GetIndex())
                {
                case epi::kENameHeightfloor:
                    fz = epi::LexInteger(value);
                    break;
                case epi::kENameHeightceiling:
                    cz = epi::LexInteger(value);
                    break;
                case epi::kENameTexturefloor:
                    epi::CStringCopyMax(floor_tex, value.c_str(), 8);
                    break;
                case epi::kENameTextureceiling:
                    epi::CStringCopyMax(ceil_tex, value.c_str(), 8);
                    break;
                case epi::kENameLightlevel:
                    light = epi::LexInteger(value);
                    break;
                case epi::kENameSpecial:
                    type = epi::LexInteger(value);
                    break;
                case epi::kENameId:
                    tag = epi::LexInteger(value);
                    break;
                case epi::kENameLightcolor:
                    light_color = ((uint32_t)epi::LexInteger(value) << 8 | 0xFF);
                    break;
                /*case epi::kENameFadecolor:
                    fog_color = ((uint32_t)epi::LexInteger(value) << 8 | 0xFF);
                    break;
                case epi::kENameFogdensity:
                    fog_density = glm_clamp(0, epi::LexInteger(value), 1020);
                    break;*/
                case epi::kENameXpanningfloor:
                    fx = epi::LexDouble(value);
                    break;
                case epi::kENameYpanningfloor:
                    fy = epi::LexDouble(value);
                    break;
                case epi::kENameXpanningceiling:
                    cx = epi::LexDouble(value);
                    break;
                case epi::kENameYpanningceiling:
                    cy = epi::LexDouble(value);
                    break;
                case epi::kENameXscalefloor:
                    fx_sc = epi::LexDouble(value);
                    break;
                case epi::kENameYscalefloor:
                    fy_sc = epi::LexDouble(value);
                    break;
                case epi::kENameXscaleceiling:
                    cx_sc = epi::LexDouble(value);
                    break;
                case epi::kENameYscaleceiling:
                    cy_sc = epi::LexDouble(value);
                    break;
                case epi::kENameAlphafloor:
                    falph = epi::LexDouble(value);
                    break;
                case epi::kENameAlphaceiling:
                    calph = epi::LexDouble(value);
                    break;
                case epi::kENameRotationfloor:
                    rf = epi::LexDouble(value);
                    break;
                case epi::kENameRotationceiling:
                    rc = epi::LexDouble(value);
                    break;
                case epi::kENameGravity:
                    gravfactor = epi::LexDouble(value);
                    break;
                case epi::kENameLiquidheight:
                    liquid_z = epi::LexInteger(value);
                    break;
                case epi::kENameLiquidcolor:
                    liquid_color = ((uint32_t)epi::LexInteger(value) << 8 | 0xFF);
                    break;
                case epi::kENameLiquidtexture:
                    epi::CStringCopyMax(liquid_tex, value.c_str(), 8);
                    break;
                case epi::kENameLiquidlight:
                    liquid_light = epi::LexInteger(value);
                    break;
                case epi::kENameLiquidtrans:
                    liquid_trans = epi::LexDouble(value);
                    break;
                default:
                    break;
                }
            }
            Sector *ss         = level_sectors + cur_sector;
            ss->floor_height   = fz;
            ss->ceiling_height = cz;

            ss->original_height = (ss->floor_height + ss->ceiling_height);

            ss->floor.translucency = falph;
            ss->floor.x_matrix.x   = 1;
            ss->floor.x_matrix.y   = 0;
            ss->floor.y_matrix.x   = 0;
            ss->floor.y_matrix.y   = 1;

            ss->ceiling = ss->deep_water_surface = ss->floor;
            ss->ceiling.translucency = calph;

            // rotations
            if (!AlmostEquals(rf, 0.0f))
                ss->floor.rotation = epi::BAMFromDegrees(rf);

            if (!AlmostEquals(rc, 0.0f))
                ss->ceiling.rotation = epi::BAMFromDegrees(rc);

            // granular scaling
            ss->floor.x_matrix.x   = fx_sc;
            ss->floor.y_matrix.y   = fy_sc;
            ss->ceiling.x_matrix.x = cx_sc;
            ss->ceiling.y_matrix.y = cy_sc;

            // granular offsets
            ss->floor.offset.x += (fx/fx_sc);
            ss->floor.offset.y -= (fy/fy_sc);
            ss->ceiling.offset.x += (cx/cx_sc);
            ss->ceiling.offset.y -= (cy/cy_sc);

            ss->floor.image = ImageLookup(floor_tex, kImageNamespaceFlat);

            if (ss->floor.image)
            {
                FlatDefinition *current_flatdef = flatdefs.Find(ss->floor.image->name_.c_str());
                if (current_flatdef)
                {
                    ss->bob_depth  = current_flatdef->bob_depth_;
                    ss->sink_depth = current_flatdef->sink_depth_;
                }
            }

            ss->ceiling.image = ImageLookup(ceil_tex, kImageNamespaceFlat);

            if (!ss->floor.image)
            {
                LogWarning("Bad Level: sector #%d has missing floor texture.\n", cur_sector);
                ss->floor.image = ImageLookup("FLAT1", kImageNamespaceFlat);
            }
            if (!ss->ceiling.image)
            {
                LogWarning("Bad Level: sector #%d has missing ceiling texture.\n", cur_sector);
                ss->ceiling.image = ss->floor.image;
            }

            // convert negative tags to zero
            ss->tag = GLM_MAX(0, tag);

            ss->properties.light_level = light;

            // convert negative types to zero
            ss->properties.type    = GLM_MAX(0, type);
            ss->properties.special = LookupSectorType(ss->properties.type);

            ss->properties.colourmap = nullptr;

            ss->properties.gravity   = kGravityDefault * gravfactor;
            ss->properties.friction  = kFrictionDefault;
            ss->properties.viscosity = kViscosityDefault;
            ss->properties.drag      = kDragDefault;

            // Allow UDMF sector light/fog information to override DDFSECT types
            /*if (fog_color != SG_BLACK_RGBA32) // All black is the established
                                              // UDMF "no fog" color
            {
                // Prevent UDMF-specified fog color from having our internal 'no
                // value'...uh...value
                if (fog_color == kRGBANoValue)
                    fog_color ^= 0x00010100;
                ss->properties.fog_color = fog_color;
                // Best-effort match for GZDoom's fogdensity values so that UDB,
                // etc give predictable results
                if (fog_density < 2)
                    ss->properties.fog_density = 0.002f;
                else
                    ss->properties.fog_density = 0.01f * ((float)fog_density / 1020.0f);
            }
            else if (ss->properties.special && ss->properties.special->fog_color_ != kRGBANoValue)
            {
                ss->properties.fog_color   = ss->properties.special->fog_color_;
                ss->properties.fog_density = 0.01f * ss->properties.special->fog_density_;
            }
            else
            {
                ss->properties.fog_color   = kRGBANoValue;
                ss->properties.fog_density = 0;
            }*/
            if (light_color != SG_WHITE_RGBA32)
            {
                if (light_color == kRGBANoValue)
                    light_color ^= 0x00010100;
                // Make colormap if necessary
                for (Colormap *cmap : colormaps)
                {
                    if (cmap->gl_color_ != kRGBANoValue && cmap->gl_color_ == light_color)
                    {
                        ss->properties.colourmap = cmap;
                        break;
                    }
                }
                if (!ss->properties.colourmap || ss->properties.colourmap->gl_color_ != light_color)
                {
                    Colormap *ad_hoc         = new Colormap;
                    ad_hoc->name_            = epi::StringFormat("UDMF_%d", light_color); // Internal
                    ad_hoc->gl_color_        = light_color;
                    ss->properties.colourmap = ad_hoc;
                    colormaps.push_back(ad_hoc);
                }
            }

            ss->active_properties = &ss->properties;

            // New to MUD: boom height replacement key/value pairs
            ss->deep_water_surface.image = ImageLookup(liquid_tex, kImageNamespaceFlat, kImageLookupNull);

            if (ss->deep_water_surface.image)
            {
                ss->has_deep_water = true;
                ss->deep_water_height = liquid_z;
                if (liquid_color == kRGBANoValue) // ensure no accidental collision
                    liquid_color ^= 0x00010100;
                // Make colormap if necessary
                for (Colormap *cmap : colormaps)
                {
                    if (cmap->gl_color_ != kRGBANoValue && cmap->gl_color_ == liquid_color)
                    {
                        ss->deep_water_properties.colourmap = cmap;
                        break;
                    }
                }
                if (!ss->deep_water_properties.colourmap || ss->properties.colourmap->gl_color_ != liquid_color)
                {
                    Colormap *ad_hoc         = new Colormap;
                    ad_hoc->name_            = epi::StringFormat("UDMF_%d", liquid_color); // Internal
                    ad_hoc->gl_color_        = liquid_color;
                    ss->deep_water_properties.colourmap = ad_hoc;
                    colormaps.push_back(ad_hoc);
                }
                ss->deep_water_properties.light_level = liquid_light;
                ss->deep_water_surface.translucency = liquid_trans;
                ss->deep_water_properties.friction = 0.9f;
                ss->deep_water_properties.viscosity = 0.7f;
                ss->deep_water_properties.gravity = 0.1f;
                ss->deep_water_properties.drag = 0.95f;
                ss->deep_water_properties.special = new SectorType();
                SectorType *water_special = (SectorType *)ss->deep_water_properties.special; // const override
                water_special->special_flags_ = (SectorFlag)(kSectorFlagDeepWater|kSectorFlagSwimming|kSectorFlagAirLess);
            }

            ss->sound_player = -1;

            // -AJA- 1999/07/29: Keep sectors with same tag in a list.
            GroupSectorTags(ss, level_sectors, cur_sector);
            cur_sector++;
        }
        else // consume other blocks
        {
            for (;;)
            {
                tok = lex.Next(section);
                if (lex.Match("}") || tok == epi::kTokenEOF)
                    break;
            }
        }
    }
    EPI_ASSERT(cur_sector == total_level_sectors);

    LogDebug("LoadUDMFSectors: finished parsing TEXTMAP\n");
}

static void LoadUDMFSideDefs()
{
    epi::Lexer lex(udmf_string);

    LogDebug("LoadUDMFSectors: parsing TEXTMAP\n");

    level_sides = new Side[total_level_sides];
    EPI_CLEAR_MEMORY(level_sides, Side, total_level_sides);

    int nummapsides = 0;

    for (;;)
    {
        std::string    section;
        epi::TokenKind tok = lex.Next(section);

        if (tok == epi::kTokenEOF)
            break;

        if (tok != epi::kTokenIdentifier)
            FatalError("Malformed TEXTMAP lump.\n");

        // check namespace
        if (lex.Match("="))
        {
            lex.Next(section);
            if (!lex.Match(";"))
                FatalError("Malformed TEXTMAP lump: missing ';'\n");
            continue;
        }

        if (!lex.Match("{"))
            FatalError("Malformed TEXTMAP lump: missing '{'\n");

        if (section == "sidedef")
        {
            nummapsides++;
            int   x = 0, y = 0;
            float lowx = 0.0f, midx = 0.0f, highx = 0.0f;
            float lowy = 0.0f, midy = 0.0f, highy = 0.0f;
            float low_scx = 1.0f, mid_scx = 1.0f, high_scx = 1.0f;
            float low_scy = 1.0f, mid_scy = 1.0f, high_scy = 1.0f;
            int   sec_num = 0;
            char  top_tex[10];
            char  bottom_tex[10];
            char  middle_tex[10];
            strcpy(top_tex, "-");
            strcpy(bottom_tex, "-");
            strcpy(middle_tex, "-");
            for (;;)
            {
                if (lex.Match("}"))
                    break;

                std::string    key;
                std::string    value;
                epi::TokenKind block_tok = lex.Next(key);

                if (block_tok == epi::kTokenEOF)
                    FatalError("Malformed TEXTMAP lump: unclosed block\n");

                if (block_tok != epi::kTokenIdentifier)
                    FatalError("Malformed TEXTMAP lump: missing key\n");

                if (!lex.Match("="))
                    FatalError("Malformed TEXTMAP lump: missing '='\n");

                block_tok = lex.Next(value);

                if (block_tok == epi::kTokenEOF || block_tok == epi::kTokenError || value == "}")
                    FatalError("Malformed TEXTMAP lump: missing value\n");

                if (!lex.Match(";"))
                    FatalError("Malformed TEXTMAP lump: missing ';'\n");

                epi::EName key_ename(key, true);

                switch (key_ename.GetIndex())
                {
                case epi::kENameOffsetx:
                    x = epi::LexInteger(value);
                    break;
                case epi::kENameOffsety:
                    y = epi::LexInteger(value);
                    break;
                case epi::kENameOffsetx_bottom:
                    lowx = epi::LexDouble(value);
                    break;
                case epi::kENameOffsetx_mid:
                    midx = epi::LexDouble(value);
                    break;
                case epi::kENameOffsetx_top:
                    highx = epi::LexDouble(value);
                    break;
                case epi::kENameOffsety_bottom:
                    lowy = epi::LexDouble(value);
                    break;
                case epi::kENameOffsety_mid:
                    midy = epi::LexDouble(value);
                    break;
                case epi::kENameOffsety_top:
                    highy = epi::LexDouble(value);
                    break;
                case epi::kENameScalex_bottom:
                    low_scx = epi::LexDouble(value);
                    break;
                case epi::kENameScalex_mid:
                    mid_scx = epi::LexDouble(value);
                    break;
                case epi::kENameScalex_top:
                    high_scx = epi::LexDouble(value);
                    break;
                case epi::kENameScaley_bottom:
                    low_scy = epi::LexDouble(value);
                    break;
                case epi::kENameScaley_mid:
                    mid_scy = epi::LexDouble(value);
                    break;
                case epi::kENameScaley_top:
                    high_scy = epi::LexDouble(value);
                    break;
                case epi::kENameTexturetop:
                    epi::CStringCopyMax(top_tex, value.c_str(), 8);
                    break;
                case epi::kENameTexturebottom:
                    epi::CStringCopyMax(bottom_tex, value.c_str(), 8);
                    break;
                case epi::kENameTexturemiddle:
                    epi::CStringCopyMax(middle_tex, value.c_str(), 8);
                    break;
                case epi::kENameSector:
                    sec_num = epi::LexInteger(value);
                    break;
                default:
                    break;
                }
            }
            EPI_ASSERT(nummapsides <= total_level_sides); // sanity check

            Side *sd = level_sides + nummapsides - 1;

            sd->top.translucency = 1.0f;
            sd->top.offset.x     = x;
            sd->top.offset.y     = y;
            sd->top.x_matrix.x   = 1;
            sd->top.x_matrix.y   = 0;
            sd->top.y_matrix.x   = 0;
            sd->top.y_matrix.y   = 1;

            sd->middle = sd->top;
            sd->bottom = sd->top;

            sd->sector = &level_sectors[sec_num];

            sd->top.image = ImageLookup(top_tex, kImageNamespaceTexture, kImageLookupNull);

            if (sd->top.image == nullptr)
                sd->top.image = ImageLookup(top_tex, kImageNamespaceTexture);

            sd->middle.image = ImageLookup(middle_tex, kImageNamespaceTexture);
            sd->bottom.image = ImageLookup(bottom_tex, kImageNamespaceTexture);

            // granular scaling
            sd->bottom.x_matrix.x = low_scx;
            sd->middle.x_matrix.x = mid_scx;
            sd->top.x_matrix.x    = high_scx;
            sd->bottom.y_matrix.y = low_scy;
            sd->middle.y_matrix.y = mid_scy;
            sd->top.y_matrix.y    = high_scy;

            // granular offsets
            sd->bottom.offset.x += lowx/low_scx;
            sd->middle.offset.x += midx/mid_scx;
            sd->top.offset.x += highx/high_scx;
            sd->bottom.offset.y += lowy/low_scy;
            sd->middle.offset.y += midy/mid_scy;
            sd->top.offset.y += highy/high_scy;
        }
        else // consume other blocks
        {
            for (;;)
            {
                tok = lex.Next(section);
                if (lex.Match("}") || tok == epi::kTokenEOF)
                    break;
            }
        }
    }

    LogDebug("LoadUDMFSideDefs: post-processing linedefs & sidedefs\n");

    // post-process linedefs & sidedefs

    EPI_ASSERT(temp_line_sides);

    Side *sd = level_sides;

    for (int i = 0; i < total_level_lines; i++)
    {
        Line *ld = level_lines + i;

        int side0 = temp_line_sides[i * 2 + 0];
        int side1 = temp_line_sides[i * 2 + 1];

        EPI_ASSERT(side0 != -1);

        if (side0 >= nummapsides)
        {
            LogWarning("Bad WAD: level %s linedef #%d has bad RIGHT side.\n", current_map->name_.c_str(), i);
            side0 = nummapsides - 1;
        }

        if (side1 != -1 && side1 >= nummapsides)
        {
            LogWarning("Bad WAD: level %s linedef #%d has bad LEFT side.\n", current_map->name_.c_str(), i);
            side1 = nummapsides - 1;
        }

        ld->side[0] = sd;
        if (sd->middle.image && (side1 != -1))
        {
            sd->middle_mask_offset = sd->middle.offset.y;
            sd->middle.offset.y    = 0;
        }
        ld->front_sector = sd->sector;
        sd->top.translucency = level_line_alphas[i];
        sd->middle.translucency = level_line_alphas[i];
        sd->bottom.translucency = level_line_alphas[i];
        sd++;

        if (side1 != -1)
        {
            ld->side[1] = sd;
            if (sd->middle.image)
            {
                sd->middle_mask_offset = sd->middle.offset.y;
                sd->middle.offset.y    = 0;
            }
            ld->back_sector = sd->sector;
            sd->top.translucency = level_line_alphas[i];
            sd->middle.translucency = level_line_alphas[i];
            sd->bottom.translucency = level_line_alphas[i];
            sd++;
        }

        EPI_ASSERT(sd <= level_sides + total_level_sides);
    }

    EPI_ASSERT(sd == level_sides + total_level_sides);

    delete[] level_line_alphas;
    level_line_alphas = nullptr;

    LogDebug("LoadUDMFSideDefs: finished parsing TEXTMAP\n");
}

static void LoadUDMFLineDefs()
{
    epi::Lexer lex(udmf_string);

    LogDebug("LoadUDMFLineDefs: parsing TEXTMAP\n");

    int cur_line = 0;

    for (;;)
    {
        std::string    section;
        epi::TokenKind tok = lex.Next(section);

        if (tok == epi::kTokenEOF)
            break;

        if (tok != epi::kTokenIdentifier)
            FatalError("Malformed TEXTMAP lump.\n");

        // check namespace
        if (lex.Match("="))
        {
            lex.Next(section);
            if (!lex.Match(";"))
                FatalError("Malformed TEXTMAP lump: missing ';'\n");
            continue;
        }

        if (!lex.Match("{"))
            FatalError("Malformed TEXTMAP lump: missing '{'\n");

        if (section == "linedef")
        {
            int flags = 0, v1 = 0, v2 = 0;
            int side0 = -1, side1 = -1, tag = -1;
            float alpha = 1.0f;
            int special = 0;
            for (;;)
            {
                if (lex.Match("}"))
                    break;

                std::string    key;
                std::string    value;
                epi::TokenKind block_tok = lex.Next(key);

                if (block_tok == epi::kTokenEOF)
                    FatalError("Malformed TEXTMAP lump: unclosed block\n");

                if (block_tok != epi::kTokenIdentifier)
                    FatalError("Malformed TEXTMAP lump: missing key\n");

                if (!lex.Match("="))
                    FatalError("Malformed TEXTMAP lump: missing '='\n");

                block_tok = lex.Next(value);

                if (block_tok == epi::kTokenEOF || block_tok == epi::kTokenError || value == "}")
                    FatalError("Malformed TEXTMAP lump: missing value\n");

                if (!lex.Match(";"))
                    FatalError("Malformed TEXTMAP lump: missing ';'\n");

                epi::EName key_ename(key, true);

                switch (key_ename.GetIndex())
                {
                case epi::kENameId:
                    tag = epi::LexInteger(value);
                    break;
                case epi::kENameV1:
                    v1 = epi::LexInteger(value);
                    break;
                case epi::kENameV2:
                    v2 = epi::LexInteger(value);
                    break;
                case epi::kENameSpecial:
                    special = epi::LexInteger(value);
                    break;
                case epi::kENameSidefront:
                    side0 = epi::LexInteger(value);
                    break;
                case epi::kENameSideback:
                    side1 = epi::LexInteger(value);
                    break;
                case epi::kENameAlpha:
                    alpha = epi::LexDouble(value);
                    break;
                case epi::kENameBlocking:
                    flags |= (epi::LexBoolean(value) ? kLineFlagBlocking : 0);
                    break;
                case epi::kENameBlockmonsters:
                    flags |= (epi::LexBoolean(value) ? kLineFlagBlockMonsters : 0);
                    break;
                case epi::kENameTwosided:
                    flags |= (epi::LexBoolean(value) ? kLineFlagTwoSided : 0);
                    break;
                case epi::kENameDontpegtop:
                    flags |= (epi::LexBoolean(value) ? kLineFlagUpperUnpegged : 0);
                    break;
                case epi::kENameDontpegbottom:
                    flags |= (epi::LexBoolean(value) ? kLineFlagLowerUnpegged : 0);
                    break;
                case epi::kENameSecret:
                    flags |= (epi::LexBoolean(value) ? kLineFlagSecret : 0);
                    break;
                case epi::kENameBlocksound:
                    flags |= (epi::LexBoolean(value) ? kLineFlagSoundBlock : 0);
                    break;
                case epi::kENameDontdraw:
                    flags |= (epi::LexBoolean(value) ? kLineFlagDontDraw : 0);
                    break;
                case epi::kENameMapped:
                    flags |= (epi::LexBoolean(value) ? kLineFlagMapped : 0);
                    break;
                case epi::kENamePassuse:
                    flags |= (epi::LexBoolean(value) ? kLineFlagBoomPassThrough : 0);
                    break;
                case epi::kENameBlockplayers:
                    flags |= (epi::LexBoolean(value) ? kLineFlagBlockPlayers : 0);
                    break;
                case epi::kENameBlocksight:
                    flags |= (epi::LexBoolean(value) ? kLineFlagSightBlock : 0);
                    break;
                default:
                    break;
                }
            }
            Line *ld = level_lines + cur_line;

            ld->flags    = flags;
            ld->tag      = GLM_MAX(0, tag);
            ld->vertex_1 = &level_vertexes[v1];
            ld->vertex_2 = &level_vertexes[v2];

            ld->special = LookupLineType(GLM_MAX(0, special));

            if (ld->special && ld->special->type_ == kLineTriggerWalkable)
                ld->flags |= kLineFlagBoomPassThrough;

            if (ld->special && ld->special->type_ == kLineTriggerNone &&
                (ld->special->s_xspeed_ || ld->special->s_yspeed_ || ld->special->scroll_type_ > BoomScrollerTypeNone ||
                 ld->special->line_effect_ == kLineEffectTypeVectorScroll ||
                 ld->special->line_effect_ == kLineEffectTypeOffsetScroll ||
                 ld->special->line_effect_ == kLineEffectTypeTaggedOffsetScroll))
                ld->flags |= kLineFlagBoomPassThrough;

            if (ld->special && ld->special == linetypes.Lookup(0)) // Add passthru to unknown/templated
                ld->flags |= kLineFlagBoomPassThrough;

            ComputeLinedefData(ld, side0, side1);

            BlockmapAddLine(ld);

            level_line_alphas[ld - level_lines] = alpha;

            cur_line++;
        }
        else // consume other blocks
        {
            for (;;)
            {
                tok = lex.Next(section);
                if (lex.Match("}") || tok == epi::kTokenEOF)
                    break;
            }
        }
    }
    EPI_ASSERT(cur_line == total_level_lines);

    LogDebug("LoadUDMFLineDefs: finished parsing TEXTMAP\n");
}

static void LoadUDMFThings()
{
    epi::Lexer lex(udmf_string);

    LogDebug("LoadUDMFThings: parsing TEXTMAP\n");
    for (;;)
    {
        std::string    section;
        epi::TokenKind tok = lex.Next(section);

        if (tok == epi::kTokenEOF)
            break;

        if (tok != epi::kTokenIdentifier)
            FatalError("Malformed TEXTMAP lump.\n");

        // check namespace
        if (lex.Match("="))
        {
            lex.Next(section);
            if (!lex.Match(";"))
                FatalError("Malformed TEXTMAP lump: missing ';'\n");
            continue;
        }

        if (!lex.Match("{"))
            FatalError("Malformed TEXTMAP lump: missing '{'\n");

        if (section == "thing")
        {
            float                      x = 0.0f, y = 0.0f, z = 0.0f;
            BAMAngle                   angle     = kBAMAngle0;
            int                        options   = kThingNotSinglePlayer | kThingNotDeathmatch | kThingNotCooperative;
            int                        typenum   = -1;
            int                        tag       = 0;
            float                      healthfac = 1.0f;
            float                      alpha     = 1.0f;
            float                      scale = 0.0f, scalex = 0.0f, scaley = 0.0f;
            const MapObjectDefinition *objtype;
            for (;;)
            {
                if (lex.Match("}"))
                    break;

                std::string    key;
                std::string    value;
                epi::TokenKind block_tok = lex.Next(key);

                if (block_tok == epi::kTokenEOF)
                    FatalError("Malformed TEXTMAP lump: unclosed block\n");

                if (block_tok != epi::kTokenIdentifier)
                    FatalError("Malformed TEXTMAP lump: missing key\n");

                if (!lex.Match("="))
                    FatalError("Malformed TEXTMAP lump: missing '='\n");

                block_tok = lex.Next(value);

                if (block_tok == epi::kTokenEOF || block_tok == epi::kTokenError || value == "}")
                    FatalError("Malformed TEXTMAP lump: missing value\n");

                if (!lex.Match(";"))
                    FatalError("Malformed TEXTMAP lump: missing ';'\n");

                epi::EName key_ename(key, true);

                switch (key_ename.GetIndex())
                {
                case epi::kENameId:
                    tag = epi::LexInteger(value);
                    break;
                case epi::kENameX:
                    x = epi::LexDouble(value);
                    break;
                case epi::kENameY:
                    y = epi::LexDouble(value);
                    break;
                case epi::kENameHeight:
                    z = epi::LexDouble(value);
                    break;
                case epi::kENameAngle:
                    angle = epi::BAMFromDegrees(epi::LexInteger(value));
                    break;
                case epi::kENameType:
                    typenum = epi::LexInteger(value);
                    break;
                case epi::kENameSkill1:
                    options |= (epi::LexBoolean(value) ? kThingEasy : 0);
                    break;
                case epi::kENameSkill2:
                    options |= (epi::LexBoolean(value) ? kThingEasy : 0);
                    break;
                case epi::kENameSkill3:
                    options |= (epi::LexBoolean(value) ? kThingMedium : 0);
                    break;
                case epi::kENameSkill4:
                    options |= (epi::LexBoolean(value) ? kThingHard : 0);
                    break;
                case epi::kENameSkill5:
                    options |= (epi::LexBoolean(value) ? kThingHard : 0);
                    break;
                case epi::kENameAmbush:
                    options |= (epi::LexBoolean(value) ? kThingAmbush : 0);
                    break;
                case epi::kENameSingle:
                    options &= (epi::LexBoolean(value) ? ~kThingNotSinglePlayer : options);
                    break;
                case epi::kENameDm:
                    options &= (epi::LexBoolean(value) ? ~kThingNotDeathmatch : options);
                    break;
                case epi::kENameCoop:
                    options &= (epi::LexBoolean(value) ? ~kThingNotCooperative : options);
                    break;
                case epi::kENameFriend:
                    options |= (epi::LexBoolean(value) ? kThingFriend : 0);
                    break;
                case epi::kENameHealth:
                    healthfac = epi::LexDouble(value);
                    break;
                case epi::kENameAlpha:
                    alpha = epi::LexDouble(value);
                    break;
                case epi::kENameScale:
                    scale = epi::LexDouble(value);
                    break;
                case epi::kENameScalex:
                    scalex = epi::LexDouble(value);
                    break;
                case epi::kENameScaley:
                    scaley = epi::LexDouble(value);
                    break;
                default:
                    break;
                }
            }
            objtype = mobjtypes.Lookup(typenum);

            // MOBJTYPE not found, don't crash out: JDS Compliance.
            // -ACB- 1998/07/21
            if (objtype == nullptr)
            {
                UnknownThingWarning(typenum, x, y);
                continue;
            }

            Sector *sec = PointInSubsector(x, y)->sector;

            if (objtype->flags_ & kMapObjectFlagSpawnCeiling)
                z += sec->ceiling_height - objtype->height_;
            else
                z += sec->floor_height;

            MapObject *udmf_thing = SpawnMapThing(objtype, x, y, z, sec, angle, options, tag);

            // check for UDMF-specific thing stuff
            if (udmf_thing)
            {
                udmf_thing->target_visibility_ = alpha;
                udmf_thing->alpha_             = alpha;
                if (!AlmostEquals(healthfac, 1.0f))
                {
                    if (healthfac < 0)
                    {
                        udmf_thing->spawn_health_ = fabs(healthfac);
                        udmf_thing->health_       = fabs(healthfac);
                    }
                    else
                    {
                        udmf_thing->spawn_health_ *= healthfac;
                        udmf_thing->health_ *= healthfac;
                    }
                }
                // Treat 'scale' and 'scalex/scaley' as one or the other; don't
                // try to juggle both
                if (!AlmostEquals(scale, 0.0f))
                {
                    udmf_thing->scale_ = scale;
                    udmf_thing->height_ *= scale;
                    udmf_thing->radius_ *= scale;
                }
                else if (!AlmostEquals(scalex, 0.0f) || !AlmostEquals(scaley, 0.0f))
                {
                    float sx           = AlmostEquals(scalex, 0.0f) ? 1.0f : scalex;
                    float sy           = AlmostEquals(scaley, 0.0f) ? 1.0f : scaley;
                    udmf_thing->scale_ = sy;
                    udmf_thing->aspect_ = (sx / sy);
                    udmf_thing->height_ *= sy;
                    udmf_thing->radius_ *= sx;
                }
            }

            total_map_things++;
        }
        else // consume other blocks
        {
            for (;;)
            {
                tok = lex.Next(section);
                if (lex.Match("}") || tok == epi::kTokenEOF)
                    break;
            }
        }
    }

    LogDebug("LoadUDMFThings: finished parsing TEXTMAP\n");
}

static void LoadUDMFCounts()
{
    epi::Lexer lex(udmf_string);

    for (;;)
    {
        std::string    section;
        epi::TokenKind tok = lex.Next(section);

        if (tok == epi::kTokenEOF)
            break;

        if (tok != epi::kTokenIdentifier)
            FatalError("Malformed TEXTMAP lump.\n");

        // check namespace
        if (lex.Match("="))
        {
            lex.Next(section);

            if (section != "doom" && section != "heretic" && section != "edge-classic" &&
                section != "zdoomtranslated")
            {
                FatalError("UDMF: %s uses unsupported namespace "
                            "\"%s\"!\nSupported namespaces are \"doom\", "
                            "\"heretic\", \"edge-classic\", or "
                            "\"zdoomtranslated\"!\n",
                            current_map->name_.c_str(), section.c_str());
            }

            if (!lex.Match(";"))
                FatalError("Malformed TEXTMAP lump: missing ';'\n");
            continue;
        }

        if (!lex.Match("{"))
            FatalError("Malformed TEXTMAP lump: missing '{'\n");

        epi::EName section_ename(section, true);

        // side counts are computed during linedef loading
        switch (section_ename.GetIndex())
        {
        case epi::kENameThing:
            total_map_things++;
            break;
        case epi::kENameVertex:
            total_level_vertexes++;
            break;
        case epi::kENameSector:
            total_level_sectors++;
            break;
        case epi::kENameLinedef:
            total_level_lines++;
            break;
        default:
            break;
        }

        // ignore block contents
        for (;;)
        {
            tok = lex.Next(section);
            if (lex.Match("}") || tok == epi::kTokenEOF)
                break;
        }
    }

    // initialize arrays
    level_vertexes = new Vertex[total_level_vertexes];
    level_sectors  = new Sector[total_level_sectors];
    EPI_CLEAR_MEMORY(level_sectors, Sector, total_level_sectors);
    level_lines = new Line[total_level_lines];
    EPI_CLEAR_MEMORY(level_lines, Line, total_level_lines);
    level_line_alphas = new float[total_level_lines];
    temp_line_sides = new int[total_level_lines * 2];
}

static void SetupSlidingDoors(void)
{
    for (int i = 0; i < total_level_lines; i++)
    {
        Line *ld = level_lines + i;

        if (!ld->special || ld->special->s_.type_ == kSlidingDoorTypeNone)
            continue;

        if (ld->tag == 0 || ld->special->type_ == kLineTriggerManual)
            ld->slide_door = ld->special;
        else
        {
            for (int k = 0; k < total_level_lines; k++)
            {
                Line *other = level_lines + k;

                if (other->tag != ld->tag || other == ld)
                    continue;

                other->slide_door = ld->special;
            }
        }
    }
}

//
// SetupVertGaps
//
// Computes how many vertical gaps we'll need.
//
static void SetupVertGaps(void)
{
    int i;
    int line_gaps       = 0;
    int sect_sight_gaps = 0;

    VerticalGap *cur_gap;

    for (i = 0; i < total_level_lines; i++)
    {
        Line *ld = level_lines + i;

        ld->maximum_gaps = ld->back_sector ? 1 : 0;

        line_gaps += ld->maximum_gaps;
    }

    for (i = 0; i < total_level_sectors; i++)
    {
        Sector *sec = level_sectors + i;

        sec->maximum_gaps = 1;

        sect_sight_gaps += sec->maximum_gaps;
    }

    total_level_vertical_gaps = line_gaps + sect_sight_gaps;

    // LogPrint("%dK used for vert gaps.\n", (total_level_vertical_gaps *
    //    sizeof(vgap_t) + 1023) / 1024);

    // zero is now impossible
    EPI_ASSERT(total_level_vertical_gaps > 0);

    level_vertical_gaps = new VerticalGap[total_level_vertical_gaps];

    EPI_CLEAR_MEMORY(level_vertical_gaps, VerticalGap, total_level_vertical_gaps);

    for (i = 0, cur_gap = level_vertical_gaps; i < total_level_lines; i++)
    {
        Line *ld = level_lines + i;

        if (ld->maximum_gaps == 0)
            continue;

        ld->gaps = cur_gap;
        cur_gap += ld->maximum_gaps;
    }

    EPI_ASSERT(cur_gap == (level_vertical_gaps + line_gaps));

    for (i = 0; i < total_level_sectors; i++)
    {
        Sector *sec = level_sectors + i;

        if (sec->maximum_gaps == 0)
            continue;

        sec->sight_gaps = cur_gap;
        cur_gap += sec->maximum_gaps;
    }

    EPI_ASSERT(cur_gap == (level_vertical_gaps + total_level_vertical_gaps));
}

//
// GroupLines
//
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void GroupLines(void)
{
    int     i;
    int     j;
    int     total;
    Line   *li;
    Sector *sector;
    Seg    *seg;
    float   bbox[4];
    Line  **line_p;

    // setup remaining seg information
    for (i = 0, seg = level_segs; i < total_level_segs; i++, seg++)
    {
        if (seg->partner)
            seg->back_subsector = seg->partner->front_subsector;

        if (!seg->front_sector)
            seg->front_sector = seg->front_subsector->sector;

        if (!seg->back_sector && seg->back_subsector)
            seg->back_sector = seg->back_subsector->sector;
    }

    // count number of lines in each sector
    li    = level_lines;
    total = 0;
    for (i = 0; i < total_level_lines; i++, li++)
    {
        total++;
        li->front_sector->line_count++;

        if (li->back_sector && li->back_sector != li->front_sector)
        {
            total++;
            li->back_sector->line_count++;
        }
    }

    // build line tables for each sector
    level_line_buffer = new Line *[total];

    line_p = level_line_buffer;
    sector = level_sectors;

    for (i = 0; i < total_level_sectors; i++, sector++)
    {
        BoundingBoxClear(bbox);
        sector->lines = line_p;
        li            = level_lines;
        for (j = 0; j < total_level_lines; j++, li++)
        {
            if (li->front_sector == sector || li->back_sector == sector)
            {
                *line_p++ = li;
                BoundingBoxAddPoint(bbox, li->vertex_1->x, li->vertex_1->y);
                BoundingBoxAddPoint(bbox, li->vertex_2->x, li->vertex_2->y);
            }
        }
        if (line_p - sector->lines != sector->line_count)
            FatalError("GroupLines: miscounted");

        // Allow vertex slope if a triangular sector or a rectangular
        // sector in which two adjacent verts have an identical z-height
        // and the other two have it unset
        if (sector->line_count == 3)
        {
            sector->floor_vertex_slope_high_low   = {{-40000, 40000}};
            sector->ceiling_vertex_slope_high_low = {{-40000, 40000}};
            for (j = 0; j < 3; j++)
            {
                Vertex *vert   = sector->lines[j]->vertex_1;
                bool    add_it = true;
                for (vec3s v : sector->floor_z_vertices)
                    if (AlmostEquals(v.x, vert->x) && AlmostEquals(v.y, vert->y))
                        add_it = false;
                if (add_it)
                {
                    if (vert->z < 32767.0f && vert->z > -32768.0f)
                    {
                        sector->floor_vertex_slope = true;
                        sector->floor_z_vertices.push_back({{vert->x, vert->y, vert->z}});
                        if (vert->z > sector->floor_vertex_slope_high_low.x)
                            sector->floor_vertex_slope_high_low.x = vert->z;
                        if (vert->z < sector->floor_vertex_slope_high_low.y)
                            sector->floor_vertex_slope_high_low.y = vert->z;
                    }
                    else
                        sector->floor_z_vertices.push_back({{vert->x, vert->y, sector->floor_height}});
                    if (vert->w < 32767.0f && vert->w > -32768.0f)
                    {
                        sector->ceiling_vertex_slope = true;
                        sector->ceiling_z_vertices.push_back({{vert->x, vert->y, vert->w}});
                        if (vert->w > sector->ceiling_vertex_slope_high_low.x)
                            sector->ceiling_vertex_slope_high_low.x = vert->w;
                        if (vert->w < sector->ceiling_vertex_slope_high_low.y)
                            sector->ceiling_vertex_slope_high_low.y = vert->w;
                    }
                    else
                        sector->ceiling_z_vertices.push_back({{vert->x, vert->y, sector->ceiling_height}});
                }
                vert   = sector->lines[j]->vertex_2;
                add_it = true;
                for (vec3s v : sector->floor_z_vertices)
                    if (AlmostEquals(v.x, vert->x) && AlmostEquals(v.y, vert->y))
                        add_it = false;
                if (add_it)
                {
                    if (vert->z < 32767.0f && vert->z > -32768.0f)
                    {
                        sector->floor_vertex_slope = true;
                        sector->floor_z_vertices.push_back({{vert->x, vert->y, vert->z}});
                        if (vert->z > sector->floor_vertex_slope_high_low.x)
                            sector->floor_vertex_slope_high_low.x = vert->z;
                        if (vert->z < sector->floor_vertex_slope_high_low.y)
                            sector->floor_vertex_slope_high_low.y = vert->z;
                    }
                    else
                        sector->floor_z_vertices.push_back({{vert->x, vert->y, sector->floor_height}});
                    if (vert->w < 32767.0f && vert->w > -32768.0f)
                    {
                        sector->ceiling_vertex_slope = true;
                        sector->ceiling_z_vertices.push_back({{vert->x, vert->y, vert->w}});
                        if (vert->w > sector->ceiling_vertex_slope_high_low.x)
                            sector->ceiling_vertex_slope_high_low.x = vert->w;
                        if (vert->w < sector->ceiling_vertex_slope_high_low.y)
                            sector->ceiling_vertex_slope_high_low.y = vert->w;
                    }
                    else
                        sector->ceiling_z_vertices.push_back({{vert->x, vert->y, sector->ceiling_height}});
                }
            }
            if (!sector->floor_vertex_slope)
                sector->floor_z_vertices.clear();
            else
            {
                sector->floor_vertex_slope_normal = TripleCrossProduct(
                    sector->floor_z_vertices[0], sector->floor_z_vertices[1], sector->floor_z_vertices[2]);
                if (sector->floor_height > sector->floor_vertex_slope_high_low.x)
                    sector->floor_vertex_slope_high_low.x = sector->floor_height;
                if (sector->floor_height < sector->floor_vertex_slope_high_low.y)
                    sector->floor_vertex_slope_high_low.y = sector->floor_height;
            }
            if (!sector->ceiling_vertex_slope)
                sector->ceiling_z_vertices.clear();
            else
            {
                sector->ceiling_vertex_slope_normal = TripleCrossProduct(
                    sector->ceiling_z_vertices[0], sector->ceiling_z_vertices[1], sector->ceiling_z_vertices[2]);
                if (sector->ceiling_height < sector->ceiling_vertex_slope_high_low.y)
                    sector->ceiling_vertex_slope_high_low.y = sector->ceiling_height;
                if (sector->ceiling_height > sector->ceiling_vertex_slope_high_low.x)
                    sector->ceiling_vertex_slope_high_low.x = sector->ceiling_height;
            }
        }
        if (sector->line_count == 4)
        {
            int floor_z_lines                     = 0;
            int ceil_z_lines                      = 0;
            sector->floor_vertex_slope_high_low   = {{-40000, 40000}};
            sector->ceiling_vertex_slope_high_low = {{-40000, 40000}};
            for (j = 0; j < 4; j++)
            {
                Vertex *vert      = sector->lines[j]->vertex_1;
                Vertex *vert2     = sector->lines[j]->vertex_2;
                bool    add_it_v1 = true;
                bool    add_it_v2 = true;
                for (vec3s v : sector->floor_z_vertices)
                    if (AlmostEquals(v.x, vert->x) && AlmostEquals(v.y, vert->y))
                        add_it_v1 = false;
                for (vec3s v : sector->floor_z_vertices)
                    if (AlmostEquals(v.x, vert2->x) && AlmostEquals(v.y, vert2->y))
                        add_it_v2 = false;
                if (add_it_v1)
                {
                    if (vert->z < 32767.0f && vert->z > -32768.0f)
                    {
                        sector->floor_z_vertices.push_back({{vert->x, vert->y, vert->z}});
                        if (vert->z > sector->floor_vertex_slope_high_low.x)
                            sector->floor_vertex_slope_high_low.x = vert->z;
                        if (vert->z < sector->floor_vertex_slope_high_low.y)
                            sector->floor_vertex_slope_high_low.y = vert->z;
                    }
                    else
                        sector->floor_z_vertices.push_back({{vert->x, vert->y, sector->floor_height}});
                    if (vert->w < 32767.0f && vert->w > -32768.0f)
                    {
                        sector->ceiling_z_vertices.push_back({{vert->x, vert->y, vert->w}});
                        if (vert->w > sector->ceiling_vertex_slope_high_low.x)
                            sector->ceiling_vertex_slope_high_low.x = vert->w;
                        if (vert->w < sector->ceiling_vertex_slope_high_low.y)
                            sector->ceiling_vertex_slope_high_low.y = vert->w;
                    }
                    else
                        sector->ceiling_z_vertices.push_back({{vert->x, vert->y, sector->ceiling_height}});
                }
                if (add_it_v2)
                {
                    if (vert2->z < 32767.0f && vert2->z > -32768.0f)
                    {
                        sector->floor_z_vertices.push_back({{vert2->x, vert2->y, vert2->z}});
                        if (vert2->z > sector->floor_vertex_slope_high_low.x)
                            sector->floor_vertex_slope_high_low.x = vert2->z;
                        if (vert2->z < sector->floor_vertex_slope_high_low.y)
                            sector->floor_vertex_slope_high_low.y = vert2->z;
                    }
                    else
                        sector->floor_z_vertices.push_back({{vert2->x, vert2->y, sector->floor_height}});
                    if (vert2->w < 32767.0f && vert2->w > -32768.0f)
                    {
                        sector->ceiling_z_vertices.push_back({{vert2->x, vert2->y, vert2->w}});
                        if (vert2->w > sector->ceiling_vertex_slope_high_low.x)
                            sector->ceiling_vertex_slope_high_low.x = vert2->w;
                        if (vert2->w < sector->ceiling_vertex_slope_high_low.y)
                            sector->ceiling_vertex_slope_high_low.y = vert2->w;
                    }
                    else
                        sector->ceiling_z_vertices.push_back({{vert2->x, vert2->y, sector->ceiling_height}});
                }
                if ((vert->z < 32767.0f && vert->z > -32768.0f) && (vert2->z < 32767.0f && vert2->z > -32768.0f) &&
                    AlmostEquals(vert->z, vert2->z))
                {
                    floor_z_lines++;
                }
                if ((vert->w < 32767.0f && vert->w > -32768.0f) && (vert2->w < 32767.0f && vert2->w > -32768.0f) &&
                    AlmostEquals(vert->w, vert2->w))
                {
                    ceil_z_lines++;
                }
            }
            if (floor_z_lines == 1 && sector->floor_z_vertices.size() == 4)
            {
                sector->floor_vertex_slope        = true;
                sector->floor_vertex_slope_normal = TripleCrossProduct(
                    sector->floor_z_vertices[0], sector->floor_z_vertices[1], sector->floor_z_vertices[2]);
                if (sector->floor_height > sector->floor_vertex_slope_high_low.x)
                    sector->floor_vertex_slope_high_low.x = sector->floor_height;
                if (sector->floor_height < sector->floor_vertex_slope_high_low.y)
                    sector->floor_vertex_slope_high_low.y = sector->floor_height;
            }
            else
                sector->floor_z_vertices.clear();
            if (ceil_z_lines == 1 && sector->ceiling_z_vertices.size() == 4)
            {
                sector->ceiling_vertex_slope        = true;
                sector->ceiling_vertex_slope_normal = TripleCrossProduct(
                    sector->ceiling_z_vertices[0], sector->ceiling_z_vertices[1], sector->ceiling_z_vertices[2]);
                if (sector->ceiling_height < sector->ceiling_vertex_slope_high_low.y)
                    sector->ceiling_vertex_slope_high_low.y = sector->ceiling_height;
                if (sector->ceiling_height > sector->ceiling_vertex_slope_high_low.x)
                    sector->ceiling_vertex_slope_high_low.x = sector->ceiling_height;
            }
            else
                sector->ceiling_z_vertices.clear();
        }

        // set the degenmobj_t to the middle of the bounding box
        sector->sound_effects_origin.x = (bbox[kBoundingBoxRight] + bbox[kBoundingBoxLeft]) / 2;
        sector->sound_effects_origin.y = (bbox[kBoundingBoxTop] + bbox[kBoundingBoxBottom]) / 2;
        sector->sound_effects_origin.z = (sector->floor_height + sector->ceiling_height) / 2;
    }
}

static inline void AddSectorToVertices(int *branches, Line *ld, Sector *sec)
{
    if (!sec)
        return;

    unsigned short sec_idx = sec - level_sectors;

    for (int vert = 0; vert < 2; vert++)
    {
        int v_idx = (vert ? ld->vertex_2 : ld->vertex_1) - level_vertexes;

        EPI_ASSERT(0 <= v_idx && v_idx < total_level_vertexes);

        if (branches[v_idx] < 0)
            continue;

        VertexSectorList *L = level_vertex_sector_lists + branches[v_idx];

        if (L->total >= kVertexSectorListMaximum)
            continue;

        int pos;

        for (pos = 0; pos < L->total; pos++)
            if (L->sectors[pos] == sec_idx)
                break;

        if (pos < L->total)
            continue; // already in there

        L->sectors[L->total++] = sec_idx;
    }
}

static void CreateVertexSeclists(void)
{
    // step 1: determine number of linedef branches at each vertex
    int *branches = new int[total_level_vertexes];

    EPI_CLEAR_MEMORY(branches, int, total_level_vertexes);

    int i;

    for (i = 0; i < total_level_lines; i++)
    {
        int v1_idx = level_lines[i].vertex_1 - level_vertexes;
        int v2_idx = level_lines[i].vertex_2 - level_vertexes;

        EPI_ASSERT(0 <= v1_idx && v1_idx < total_level_vertexes);
        EPI_ASSERT(0 <= v2_idx && v2_idx < total_level_vertexes);

        branches[v1_idx] += 1;
        branches[v2_idx] += 1;
    }

    // step 2: count how many vertices have 3 or more branches,
    //         and simultaneously give them index numbers.
    int num_triples = 0;

    for (i = 0; i < total_level_vertexes; i++)
    {
        if (branches[i] < 3)
            branches[i] = -1;
        else
            branches[i] = num_triples++;
    }

    if (num_triples == 0)
    {
        delete[] branches;

        level_vertex_sector_lists = nullptr;
        return;
    }

    // step 3: create a vertex_seclist for those multi-branches
    level_vertex_sector_lists = new VertexSectorList[num_triples];

    EPI_CLEAR_MEMORY(level_vertex_sector_lists, VertexSectorList, num_triples);

    LogDebug("Created %d seclists from %d vertices (%1.1f%%)\n", num_triples, total_level_vertexes,
             num_triples * 100 / (float)total_level_vertexes);

    for (i = 0; i < total_level_lines; i++)
    {
        Line *ld = level_lines + i;

        for (int side = 0; side < 2; side++)
        {
            Sector *sec = side ? ld->back_sector : ld->front_sector;

            AddSectorToVertices(branches, ld, sec);
        }
    }

    // step 4: finally, update the segs that touch those vertices
    for (i = 0; i < total_level_segs; i++)
    {
        Seg *sg = level_segs + i;

        for (int vert = 0; vert < 2; vert++)
        {
            int v_idx = (vert ? sg->vertex_2 : sg->vertex_1) - level_vertexes;

            // skip GL vertices
            if (v_idx < 0 || v_idx >= total_level_vertexes)
                continue;

            if (branches[v_idx] < 0)
                continue;

            sg->vertex_sectors[vert] = level_vertex_sector_lists + branches[v_idx];
        }
    }

    delete[] branches;
}

static void P_RemoveSectorStuff(void)
{
    int i;

    for (i = 0; i < total_level_sectors; i++)
    {
        FreeSectorTouchNodes(level_sectors + i);

        // Might still be playing a sound.
        StopSoundEffect(&level_sectors[i].sound_effects_origin);
    }
}

void ShutdownLevel(void)
{
    // Destroys everything on the level.

#ifdef DEVELOPERS
    if (!level_active)
        FatalError("ShutdownLevel: no level to shut down!");
#endif

    level_active = false;

    ClearRespawnQueue();

    P_RemoveSectorStuff();

    StopLevelSoundEffects();

    DestroyAllForces();
    DestroyAllLights();
    DestroyAllPlanes();
    DestroyAllSliders();
    DestroyAllAmbientSounds();

    delete[] level_segs;
    level_segs = nullptr;
    delete[] level_nodes;
    level_nodes = nullptr;
    delete[] level_vertexes;
    level_vertexes = nullptr;
    delete[] level_sides;
    level_sides = nullptr;
    delete[] level_lines;
    level_lines = nullptr;
    delete[] level_sectors;
    level_sectors = nullptr;
    delete[] level_subsectors;
    level_subsectors = nullptr;
    delete[] level_gl_vertexes;
    level_gl_vertexes = nullptr;
    delete[] level_vertical_gaps;
    level_vertical_gaps = nullptr;
    delete[] level_line_buffer;
    level_line_buffer = nullptr;
    delete[] level_vertex_sector_lists;
    level_vertex_sector_lists = nullptr;

    DestroyBlockmap();

    RemoveAllMapObjects(false);
}

void LevelSetup(void)
{
    // Sets up the current level using the skill passed and the
    // information in current_map.
    //
    // -ACB- 1998/08/09 Use current_map to ref lump and par time

    if (level_active)
        ShutdownLevel();

    // -ACB- 1998/08/27 nullptr the head pointers for the linked lists....
    respawn_queue_head   = nullptr;
    map_object_list_head = nullptr;
    seen_monsters.clear();
    udmf_string.clear();
    node_file.clear();

    epi::File *udmf_file = OpenPackFile(epi::StringFormat("%s.txt", current_map->name_.c_str()), "maps");
    if (!udmf_file)
        FatalError("No such level: maps/%s.txt\n", current_map->name_.c_str());
    udmf_string = udmf_file->ReadAsString();
    delete udmf_file;

    if (udmf_string.empty())
        FatalError("Internal error: can't load UDMF lump.\n");

    // This needs to be cached somewhere, but it will work here while developing - Dasho
    uint64_t udmf_hash = epi::StringHash64(udmf_string);
    node_file = epi::PathAppend("cache", epi::StringFormat("%s-%lu.xgl", current_map->name_.c_str(), udmf_hash));

    // get lump for XGL3 nodes from an XWA file
    // shouldn't happen (as during startup we checked for or built these)
    if (!epi::FileExists(node_file))
        FatalError("Internal error: Missing node file %s.\n", node_file.c_str());

    // note: most of this ordering is important
    // 23-6-98 KM, eg, Sectors must be loaded before sidedefs,
    // Vertexes must be loaded before LineDefs,
    // LineDefs + Vertexes must be loaded before BlockMap,
    // Sectors must be loaded before Segs

    total_level_sides         = 0;
    total_level_vertical_gaps = 0;
    total_map_things          = 0;
    total_level_vertexes      = 0;
    total_level_sectors       = 0;
    total_level_lines         = 0;

    LoadUDMFCounts();
    LoadUDMFVertexes();
    LoadUDMFSectors();
    LoadUDMFLineDefs();
    LoadUDMFSideDefs();

    SetupSlidingDoors();
    SetupVertGaps();

    delete[] temp_line_sides;

    LoadXGL3Nodes();

    GroupLines();

    ComputeSkyHeights();

    // compute sector and line gaps
    for (int j = 0; j < total_level_sectors; j++)
        RecomputeGapsAroundSector(level_sectors + j);

    ClearBodyQueue();

    // set up world state
    SpawnMapSpecials1();

    // -AJA- 1999/10/21: Clear out player starts (ready to load).
    ClearPlayerStarts();

    unknown_thing_map.clear();

    LoadUDMFThings();

    CreateVertexSeclists();

    SpawnMapSpecials2(current_map->autotag_);

    UpdateSkyboxTextures();

    // preload graphics
    if (precache)
        PrecacheLevelGraphics();

    // setup categories based on game mode (SP/COOP/DM)
    UpdateSoundCategoryLimits();

    ChangeMusic(current_map->music_, true); // start level music

    level_active = true;
}

void PlayerStateInit(void)
{
    StartupProgressMessage(language["PlayState"]);

    // There should not yet exist a player
    EPI_ASSERT(total_players == 0);

    ClearPlayerStarts();
}

LineType *LookupLineType(int num)
{
    if (num <= 0)
        return nullptr;

    LineType *def = linetypes.Lookup(num);

    // DDF types always override
    if (def)
        return def;

    LogWarning("P_LookupLineType(): Unknown linedef type %d\n", num);

    return linetypes.Lookup(0); // template line
}

SectorType *LookupSectorType(int num)
{
    if (num <= 0)
        return nullptr;

    SectorType *def = sectortypes.Lookup(num);

    // DDF types always override
    if (def)
        return def;

    LogWarning("P_LookupSectorType(): Unknown sector type %d\n", num);

    return sectortypes.Lookup(0); // template sector
}

void LevelShutdown(void)
{
    if (level_active)
    {
        ShutdownLevel();
    }
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
