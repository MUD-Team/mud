//------------------------------------------------------------------------
//
//  AJ-BSP  Copyright (C) 2000-2023  Andrew Apted, et al
//          Copyright (C) 1994-1998  Colin Reed
//          Copyright (C) 1997-1998  Lee Killough
//
//  Originally based on the program 'BSP', version 2.3.
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
//------------------------------------------------------------------------

#include <algorithm>

#include "bsp_local.h"
#include "bsp_utility.h"
#include "epi_ename.h"
#include "epi_endian.h"
#include "epi_filesystem.h"
#include "epi_lexer.h"
#include "epi_str_util.h"

#define AJBSP_DEBUG_BLOCKMAP 0
#define AJBSP_DEBUG_REJECT   0

#define AJBSP_DEBUG_LOAD 0
#define AJBSP_DEBUG_BSP  0

// Startup Messages
extern void StartupProgressMessage(const char *message);

namespace ajbsp
{

enum UDMFTypes
{
    kUDMFThing   = 1,
    kUDMFVertex  = 2,
    kUDMFSector  = 3,
    kUDMFSidedef = 4,
    kUDMFLinedef = 5
};

struct RawV2Vertex
{
    int32_t x, y;
};

struct RawBoundingBox
{
    int16_t maximum_y, minimum_y;
    int16_t minimum_x, maximum_x;
};
struct RawV5Node
{
    // this structure used by ZDoom nodes too

    int16_t        x, y;                           // starting point
    int16_t        delta_x, delta_y;               // offset to ending point
    RawBoundingBox bounding_box_1, bounding_box_2; // bounding rectangles
    uint32_t       right, left;                    // children: Node or SSector (if high bit is set)
};

BuildInfo current_build_info;
static std::string current_map_name;

//----------------------------------------------------------------------

static epi::File *xgl_out = nullptr;

static void XGL3BeginLump(epi::File *out_file)
{
    xgl_out = out_file;
}

static void XGL3AppendLump(const void *data, int length)
{
    xgl_out->Write(data, length);
}

static void XGL3FinishLump(void)
{
    xgl_out = nullptr;
}


//------------------------------------------------------------------------
// LEVEL : Level structure read/write functions.
//------------------------------------------------------------------------

// Note: ZDoom format support based on code (C) 2002,2003 Randy Heit

// objects of loaded level, and stuff we've built
std::vector<Vertex *>  level_vertices;
std::vector<Linedef *> level_linedefs;
std::vector<Sidedef *> level_sidedefs;
std::vector<Sector *>  level_sectors;
std::vector<Thing *>   level_things;

std::vector<Seg *>       level_segs;
std::vector<Subsector *> level_subsecs;
std::vector<Node *>      level_nodes;
std::vector<WallTip *>   level_walltips;

int num_old_vert   = 0;
int num_new_vert   = 0;
int num_real_lines = 0;

/* ----- allocation routines ---------------------------- */

Vertex *NewVertex()
{
    Vertex *V = (Vertex *)UtilCalloc(sizeof(Vertex));
    V->index_ = (int)level_vertices.size();
    level_vertices.push_back(V);
    return V;
}

Linedef *NewLinedef()
{
    Linedef *L = (Linedef *)UtilCalloc(sizeof(Linedef));
    L->index   = (int)level_linedefs.size();
    level_linedefs.push_back(L);
    return L;
}

Sidedef *NewSidedef()
{
    Sidedef *S = (Sidedef *)UtilCalloc(sizeof(Sidedef));
    S->index   = (int)level_sidedefs.size();
    level_sidedefs.push_back(S);
    return S;
}

Sector *NewSector()
{
    Sector *S = (Sector *)UtilCalloc(sizeof(Sector));
    S->index  = (int)level_sectors.size();
    level_sectors.push_back(S);
    return S;
}

Thing *NewThing()
{
    Thing *T = (Thing *)UtilCalloc(sizeof(Thing));
    T->index = (int)level_things.size();
    level_things.push_back(T);
    return T;
}

Seg *NewSeg()
{
    Seg *S = (Seg *)UtilCalloc(sizeof(Seg));
    level_segs.push_back(S);
    return S;
}

Subsector *NewSubsec()
{
    Subsector *S = (Subsector *)UtilCalloc(sizeof(Subsector));
    level_subsecs.push_back(S);
    return S;
}

Node *NewNode()
{
    Node *N = (Node *)UtilCalloc(sizeof(Node));
    level_nodes.push_back(N);
    return N;
}

WallTip *NewWallTip()
{
    WallTip *WT = (WallTip *)UtilCalloc(sizeof(WallTip));
    level_walltips.push_back(WT);
    return WT;
}

/* ----- free routines ---------------------------- */

void FreeVertices()
{
    for (unsigned int i = 0; i < level_vertices.size(); i++)
        UtilFree((void *)level_vertices[i]);

    level_vertices.clear();
}

void FreeLinedefs()
{
    for (unsigned int i = 0; i < level_linedefs.size(); i++)
        UtilFree((void *)level_linedefs[i]);

    level_linedefs.clear();
}

void FreeSidedefs()
{
    for (unsigned int i = 0; i < level_sidedefs.size(); i++)
        UtilFree((void *)level_sidedefs[i]);

    level_sidedefs.clear();
}

void FreeSectors()
{
    for (unsigned int i = 0; i < level_sectors.size(); i++)
        UtilFree((void *)level_sectors[i]);

    level_sectors.clear();
}

void FreeThings()
{
    for (unsigned int i = 0; i < level_things.size(); i++)
        UtilFree((void *)level_things[i]);

    level_things.clear();
}

void FreeSegs()
{
    for (unsigned int i = 0; i < level_segs.size(); i++)
        UtilFree((void *)level_segs[i]);

    level_segs.clear();
}

void FreeSubsecs()
{
    for (unsigned int i = 0; i < level_subsecs.size(); i++)
        UtilFree((void *)level_subsecs[i]);

    level_subsecs.clear();
}

void FreeNodes()
{
    for (unsigned int i = 0; i < level_nodes.size(); i++)
        UtilFree((void *)level_nodes[i]);

    level_nodes.clear();
}

void FreeWallTips()
{
    for (unsigned int i = 0; i < level_walltips.size(); i++)
        UtilFree((void *)level_walltips[i]);

    level_walltips.clear();
}

/* ----- reading routines ------------------------------ */

static Vertex *SafeLookupVertex(int num)
{
    if (num >= level_vertices.size())
        FatalError("AJBSP: illegal vertex number #%d\n", num);

    return level_vertices[num];
}

static inline Sidedef *SafeLookupSidedef(int num)
{
    // silently ignore illegal sidedef numbers
    if (num >= (unsigned int)level_sidedefs.size())
        return nullptr;

    return level_sidedefs[num];
}

/* ----- UDMF reading routines ------------------------- */

void ParseThingField(Thing *thing, const int &key, const std::string &value)
{
    if (key == epi::kENameX)
        thing->x = RoundToInteger(epi::LexDouble(value));
    else if (key == epi::kENameY)
        thing->y = RoundToInteger(epi::LexDouble(value));
    else if (key == epi::kENameType)
        thing->type = epi::LexInteger(value);
}

void ParseVertexField(Vertex *vertex, const int &key, const std::string &value)
{
    if (key == epi::kENameX)
        vertex->x_ = epi::LexDouble(value);
    else if (key == epi::kENameY)
        vertex->y_ = epi::LexDouble(value);
}

void ParseSidedefField(Sidedef *side, const int &key, const std::string &value)
{
    if (key == epi::kENameSector)
    {
        int num = epi::LexInteger(value);

        if (num < 0 || num >= level_sectors.size())
            FatalError("AJBSP: illegal sector number #%d\n", (int)num);

        side->sector = level_sectors[num];
    }
}

void ParseLinedefField(Linedef *line, const int &key, const std::string &value)
{
    switch (key)
    {
    case epi::kENameV1:
        line->start = SafeLookupVertex(epi::LexInteger(value));
        break;
    case epi::kENameV2:
        line->end = SafeLookupVertex(epi::LexInteger(value));
        break;
    case epi::kENameSpecial:
        line->type = epi::LexInteger(value);
        break;
    case epi::kENameTwosided:
        line->two_sided = epi::LexBoolean(value);
        break;
    case epi::kENameSidefront: {
        int num = epi::LexInteger(value);

        if (num < 0 || num >= (int)level_sidedefs.size())
            line->right = nullptr;
        else
            line->right = level_sidedefs[num];
    }
    break;
    case epi::kENameSideback: {
        int num = epi::LexInteger(value);

        if (num < 0 || num >= (int)level_sidedefs.size())
            line->left = nullptr;
        else
            line->left = level_sidedefs[num];
    }
    break;
    default:
        break;
    }
}

void ParseUDMF_Block(epi::Lexer &lex, int cur_type)
{
    Vertex  *vertex = nullptr;
    Thing   *thing  = nullptr;
    Sidedef *side   = nullptr;
    Linedef *line   = nullptr;

    switch (cur_type)
    {
    case kUDMFVertex:
        vertex = NewVertex();
        break;
    case kUDMFThing:
        thing = NewThing();
        break;
    case kUDMFSector:
        NewSector(); // We don't use the returned pointer in this function
        break;
    case kUDMFSidedef:
        side = NewSidedef();
        break;
    case kUDMFLinedef:
        line = NewLinedef();
        break;
    default:
        break;
    }

    for (;;)
    {
        if (lex.Match("}"))
            break;

        std::string key;
        std::string value;

        epi::TokenKind tok = lex.Next(key);

        if (tok == epi::kTokenEOF)
            FatalError("AJBSP: Malformed TEXTMAP lump: unclosed block\n");

        if (tok != epi::kTokenIdentifier)
            FatalError("AJBSP: Malformed TEXTMAP lump: missing key\n");

        if (!lex.Match("="))
            FatalError("AJBSP: Malformed TEXTMAP lump: missing '='\n");

        tok = lex.Next(value);

        if (tok == epi::kTokenEOF || tok == epi::kTokenError || value == "}")
            FatalError("AJBSP: Malformed TEXTMAP lump: missing value\n");

        if (!lex.Match(";"))
            FatalError("AJBSP: Malformed TEXTMAP lump: missing ';'\n");

        epi::EName key_ename(key, true);

        switch (cur_type)
        {
        case kUDMFVertex:
            ParseVertexField(vertex, key_ename.GetIndex(), value);
            break;
        case kUDMFThing:
            ParseThingField(thing, key_ename.GetIndex(), value);
            break;
        case kUDMFSidedef:
            ParseSidedefField(side, key_ename.GetIndex(), value);
            break;
        case kUDMFLinedef:
            ParseLinedefField(line, key_ename.GetIndex(), value);
            break;
        case kUDMFSector:
        default: /* just skip it */
            break;
        }
    }

    // validate stuff

    if (line != nullptr)
    {
        if (line->start == nullptr || line->end == nullptr)
            FatalError("AJBSP: Linedef #%d is missing a vertex!\n", line->index);

        if (line->right || line->left)
            num_real_lines++;

        line->self_referencing = (line->left && line->right && (line->left->sector == line->right->sector));

        if (line->self_referencing)
            LogWarning("AJBSP: Map %s has self-referencing linedefs, which are not supported!\n", current_map_name.c_str());
    }
}

void ParseUDMF_Pass(const std::string &data, int pass)
{
    // pass = 0 : namespace/basic structure validation
    // pass = 1 : vertices, sectors, things
    // pass = 2 : sidedefs
    // pass = 3 : linedefs

    epi::Lexer lex(data);

    for (;;)
    {
        std::string    section;
        epi::TokenKind tok = lex.Next(section);

        if (tok == epi::kTokenEOF)
            return;

        if (tok != epi::kTokenIdentifier)
        {
            FatalError("AJBSP: Malformed TEXTMAP lump.\n");
            return;
        }

        if (lex.Match("="))
        {
            lex.Next(section);

            if (pass == 0)
            {
                if (section != "doom" && section != "heretic" && section != "edge-classic" &&
                        section != "zdoomtranslated")
                {
                    FatalError("UDMF: %s uses unsupported namespace "
                                "\"%s\"!\nSupported namespaces are \"doom\", "
                                "\"heretic\", \"edge-classic\", or "
                                "\"zdoomtranslated\"!\n",
                                current_map_name.c_str(), section.c_str());
                }
            }

            if (!lex.Match(";"))
                FatalError("AJBSP: Malformed TEXTMAP lump: missing ';'\n");
            continue;
        }

        if (!lex.Match("{"))
            FatalError("AJBSP: Malformed TEXTMAP lump: missing '{'\n");

        if (pass == 0)
            return;

        int cur_type = 0;

        epi::EName section_ename(section, true);

        switch (section_ename.GetIndex())
        {
        case epi::kENameThing:
            if (pass == 1)
                cur_type = kUDMFThing;
            break;
        case epi::kENameVertex:
            if (pass == 1)
                cur_type = kUDMFVertex;
            break;
        case epi::kENameSector:
            if (pass == 1)
                cur_type = kUDMFSector;
            break;
        case epi::kENameSidedef:
            if (pass == 2)
                cur_type = kUDMFSidedef;
            break;
        case epi::kENameLinedef:
            if (pass == 3)
                cur_type = kUDMFLinedef;
            break;
        default:
            break;
        }

        // process the block
        ParseUDMF_Block(lex, cur_type);
    }
}

void ParseUDMF(const std::string &textmap)
{
    if (textmap.empty())
        FatalError("AJBSP: Empty TEXTMAP lump?\n");

    // the UDMF spec does not require objects to be in a dependency order.
    // for example: sidedefs may occur *after* the linedefs which refer to
    // them.  hence we perform multiple passes over the TEXTMAP data.

    ParseUDMF_Pass(textmap, 0);
    ParseUDMF_Pass(textmap, 1);
    ParseUDMF_Pass(textmap, 2);
    ParseUDMF_Pass(textmap, 3);

    num_old_vert = level_vertices.size();
}

/* ----- writing routines ------------------------------ */

static inline uint32_t VertexIndex_XNOD(const Vertex *v)
{
    if (v->is_new_)
        return (uint32_t)(num_old_vert + v->index_);

    return (uint32_t)v->index_;
}

struct CompareSegPredicate
{
    inline bool operator()(const Seg *A, const Seg *B) const
    {
        return A->index_ < B->index_;
    }
};

void SortSegs()
{
    // do a sanity check
    for (int i = 0; i < level_segs.size(); i++)
        if (level_segs[i]->index_ < 0)
            FatalError("AJBSP: Seg %d never reached a subsector!\n", i);

    // sort segs into ascending index
    std::sort(level_segs.begin(), level_segs.end(), CompareSegPredicate());

    // remove unwanted segs
    while (level_segs.size() > 0 && level_segs.back()->index_ == kSegIsGarbage)
    {
        UtilFree((void *)level_segs.back());
        level_segs.pop_back();
    }
}

/* ----- ZDoom format writing --------------------------- */

static const uint8_t *level_XGL3_magic = (uint8_t *)"XGL3";

void PutZVertices()
{
    int count, i;

    uint32_t orgverts = AlignedLittleEndianU32(num_old_vert);
    uint32_t newverts = AlignedLittleEndianU32(num_new_vert);

    XGL3AppendLump(&orgverts, 4);
    XGL3AppendLump(&newverts, 4);

    for (i = 0, count = 0; i < level_vertices.size(); i++)
    {
        RawV2Vertex raw;

        const Vertex *vert = level_vertices[i];

        if (!vert->is_new_)
            continue;

        raw.x = AlignedLittleEndianS32(RoundToInteger(vert->x_ * 65536.0));
        raw.y = AlignedLittleEndianS32(RoundToInteger(vert->y_ * 65536.0));

        XGL3AppendLump(&raw, sizeof(raw));

        count++;
    }

    if (count != num_new_vert)
        FatalError("AJBSP: PutZVertices miscounted (%d != %d)\n", count, num_new_vert);
}

void PutZSubsecs()
{
    uint32_t Rawnum = AlignedLittleEndianU32(level_subsecs.size());
    XGL3AppendLump(&Rawnum, 4);

    int cur_seg_index = 0;

    for (int i = 0; i < level_subsecs.size(); i++)
    {
        const Subsector *sub = level_subsecs[i];

        Rawnum = AlignedLittleEndianU32(sub->seg_count_);
        XGL3AppendLump(&Rawnum, 4);

        // sanity check the seg index values
        int count = 0;
        for (const Seg *seg = sub->seg_list_; seg; seg = seg->next_, cur_seg_index++)
        {
            if (cur_seg_index != seg->index_)
                FatalError("AJBSP: PutZSubsecs: seg index mismatch in sub %d (%d != "
                           "%d)\n",
                           i, cur_seg_index, seg->index_);

            count++;
        }

        if (count != sub->seg_count_)
            FatalError("AJBSP: PutZSubsecs: miscounted segs in sub %d (%d != %d)\n", i, count, sub->seg_count_);
    }

    if (cur_seg_index != level_segs.size())
        FatalError("AJBSP: PutZSubsecs miscounted segs (%d != %d)\n", cur_seg_index, level_segs.size());
}

void PutZSegs()
{
    uint32_t Rawnum = AlignedLittleEndianU32(level_segs.size());
    XGL3AppendLump(&Rawnum, 4);

    for (int i = 0; i < level_segs.size(); i++)
    {
        const Seg *seg = level_segs[i];

        if (seg->index_ != i)
            FatalError("AJBSP: PutZSegs: seg index mismatch (%d != %d)\n", seg->index_, i);

        uint32_t v1 = AlignedLittleEndianU32(VertexIndex_XNOD(seg->start_));
        uint32_t v2 = AlignedLittleEndianU32(VertexIndex_XNOD(seg->end_));

        uint16_t line = AlignedLittleEndianU16(seg->linedef_->index);
        uint8_t  side = (uint8_t)seg->side_;

        XGL3AppendLump(&v1, 4);
        XGL3AppendLump(&v2, 4);
        XGL3AppendLump(&line, 2);
        XGL3AppendLump(&side, 1);
    }
}

void PutXGL3Segs()
{
    uint32_t Rawnum = AlignedLittleEndianU32(level_segs.size());
    XGL3AppendLump(&Rawnum, 4);

    for (int i = 0; i < level_segs.size(); i++)
    {
        const Seg *seg = level_segs[i];

        if (seg->index_ != i)
            FatalError("AJBSP: PutXGL3Segs: seg index mismatch (%d != %d)\n", seg->index_, i);

        uint32_t v1      = AlignedLittleEndianU32(VertexIndex_XNOD(seg->start_));
        uint32_t partner = AlignedLittleEndianU32(seg->partner_ ? seg->partner_->index_ : -1);
        uint32_t line    = AlignedLittleEndianU32(seg->linedef_ ? seg->linedef_->index : -1);
        uint8_t  side    = (uint8_t)seg->side_;

        XGL3AppendLump(&v1, 4);
        XGL3AppendLump(&partner, 4);
        XGL3AppendLump(&line, 4);
        XGL3AppendLump(&side, 1);

#if AJBSP_DEBUG_BSP
        fprintf(stderr, "SEG[%d] v1=%d partner=%d line=%d side=%d\n", i, v1, partner, line, side);
#endif
    }
}

static int node_cur_index;

static void PutOneZNode(Node *node)
{
    RawV5Node raw;

    if (node->r_.node)
        PutOneZNode(node->r_.node);

    if (node->l_.node)
        PutOneZNode(node->l_.node);

    node->index_ = node_cur_index++;

    uint32_t x  = AlignedLittleEndianS32(RoundToInteger(node->x_ * 65536.0));
    uint32_t y  = AlignedLittleEndianS32(RoundToInteger(node->y_ * 65536.0));
    uint32_t dx = AlignedLittleEndianS32(RoundToInteger(node->dx_ * 65536.0));
    uint32_t dy = AlignedLittleEndianS32(RoundToInteger(node->dy_ * 65536.0));

    XGL3AppendLump(&x, 4);
    XGL3AppendLump(&y, 4);
    XGL3AppendLump(&dx, 4);
    XGL3AppendLump(&dy, 4);

    raw.bounding_box_1.minimum_x = AlignedLittleEndianS16(node->r_.bounds.minimum_x);
    raw.bounding_box_1.minimum_y = AlignedLittleEndianS16(node->r_.bounds.minimum_y);
    raw.bounding_box_1.maximum_x = AlignedLittleEndianS16(node->r_.bounds.maximum_x);
    raw.bounding_box_1.maximum_y = AlignedLittleEndianS16(node->r_.bounds.maximum_y);

    raw.bounding_box_2.minimum_x = AlignedLittleEndianS16(node->l_.bounds.minimum_x);
    raw.bounding_box_2.minimum_y = AlignedLittleEndianS16(node->l_.bounds.minimum_y);
    raw.bounding_box_2.maximum_x = AlignedLittleEndianS16(node->l_.bounds.maximum_x);
    raw.bounding_box_2.maximum_y = AlignedLittleEndianS16(node->l_.bounds.maximum_y);

    XGL3AppendLump(&raw.bounding_box_1, sizeof(raw.bounding_box_1));
    XGL3AppendLump(&raw.bounding_box_2, sizeof(raw.bounding_box_2));

    if (node->r_.node)
        raw.right = AlignedLittleEndianU32(node->r_.node->index_);
    else if (node->r_.subsec)
        raw.right = AlignedLittleEndianU32(node->r_.subsec->index_ | 0x80000000U);
    else
        FatalError("AJBSP: Bad right child in V5 node %d\n", node->index_);

    if (node->l_.node)
        raw.left = AlignedLittleEndianU32(node->l_.node->index_);
    else if (node->l_.subsec)
        raw.left = AlignedLittleEndianU32(node->l_.subsec->index_ | 0x80000000U);
    else
        FatalError("AJBSP: Bad left child in V5 node %d\n", node->index_);

    XGL3AppendLump(&raw.right, 4);
    XGL3AppendLump(&raw.left, 4);

#if AJBSP_DEBUG_BSP
    LogDebug("PUT Z NODE %08X  Left %08X  Right %08X  "
             "(%d,%d) -> (%d,%d)\n",
             node->index, AlignedLittleEndianU32(raw.left), AlignedLittleEndianU32(raw.right), node->x_, node->y_,
             node->x_ + node->dx_, node->y_ + node->dy_);
#endif
}

void PutZNodes(Node *root)
{
    uint32_t Rawnum = AlignedLittleEndianU32(level_nodes.size());
    XGL3AppendLump(&Rawnum, 4);

    node_cur_index = 0;

    if (root)
        PutOneZNode(root);

    if (node_cur_index != level_nodes.size())
        FatalError("AJBSP: PutZNodes miscounted (%d != %d)\n", node_cur_index, level_nodes.size());
}

void SaveXGL3Format(epi::File *nodes_out, Node *root_node)
{
    nodes_out->Write(level_XGL3_magic, 4);

    XGL3BeginLump(nodes_out);

    PutZVertices();
    PutZSubsecs();
    PutXGL3Segs();
    PutZNodes(root_node);

    XGL3FinishLump();
}

/* ----- whole-level routines --------------------------- */

void LoadLevel(const std::string &textmap)
{
    StartupProgressMessage(epi::StringFormat("Building nodes for %s\n", current_map_name.c_str()).c_str());

    num_new_vert   = 0;
    num_real_lines = 0;

    ParseUDMF(textmap);

    LogDebug("    Loaded %d vertices, %d sectors, %d sides, %d lines, %d things\n", level_vertices.size(),
             level_sectors.size(), level_sidedefs.size(), level_linedefs.size(), level_things.size());

    DetectOverlappingVertices();
    DetectOverlappingLines();

    CalculateWallTips();
}

void FreeLevel()
{
    FreeVertices();
    FreeSidedefs();
    FreeLinedefs();
    FreeSectors();
    FreeThings();
    FreeSegs();
    FreeSubsecs();
    FreeNodes();
    FreeWallTips();
    FreeIntersections();
}

BuildResult SaveXWA(const std::string &filename, Node *root_node)
{
    epi::File *nodes_out = epi::FileOpen(filename, epi::kFileAccessWrite);

    if (!nodes_out)
        FatalError("AJBSP: Failed to open %s for writing!\n", std::string(filename).c_str());

    if (num_real_lines == 0)
        FatalError("AJBSP: %s is for an empty level?\n", std::string(filename).c_str());
    else
    {
        SortSegs();
        SaveXGL3Format(nodes_out, root_node);
    }

    delete nodes_out;

    return kBuildOK;
}

//------------------------------------------------------------------------
// MAIN STUFF
//------------------------------------------------------------------------

void ResetInfo()
{
    current_build_info.total_minor_issues = 0;
    current_build_info.total_warnings     = 0;
    current_build_info.split_cost         = kSplitCostDefault;
    current_map_name.clear();
}

/* ----- build nodes for a single UDMF level ----- */

BuildResult BuildLevel(std::string_view mapname, const std::string &filename, const std::string &textmap)
{
    Node      *root_node = nullptr;
    Subsector *root_sub  = nullptr;

    current_map_name = mapname;

    LoadLevel(textmap);

    BuildResult ret = kBuildOK;

    if (num_real_lines > 0)
    {
        BoundingBox dummy;

        // create initial segs
        Seg *list = CreateSegs();

        // recursively create nodes
        ret = BuildNodes(list, 0, &dummy, &root_node, &root_sub);
    }

    if (ret == kBuildOK)
    {
        LogDebug("    Built %d NODES, %d SSECTORS, %d SEGS, %d VERTEXES\n", level_nodes.size(), level_subsecs.size(),
                 level_segs.size(), num_old_vert + num_new_vert);

        if (root_node != nullptr)
        {
            LogDebug("    Heights of subtrees: %d / %d\n", ComputeBSPHeight(root_node->r_.node),
                     ComputeBSPHeight(root_node->l_.node));
        }

        ClockwiseBSPTree();

        if (!filename.empty())
            ret = SaveXWA(filename, root_node);
        else
            FatalError("AJBSP: Cannot save nodes to %s!\n", std::string(filename).c_str());
    }
    else
    {
        FatalError("AJBSP: Failed building %s!\n", std::string(filename).c_str());
    }

    FreeLevel();

    return ret;
}

} // namespace ajbsp

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
