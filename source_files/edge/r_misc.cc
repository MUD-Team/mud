//----------------------------------------------------------------------------
//  EDGE Main Rendering Organisation Code
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
//
// -KM- 1998/09/27 Dynamic Colourmaps
//

#include "r_misc.h"

#include <math.h>

#include "AlmostEquals.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_main.h"
#include "epi.h"
#include "i_defs_gl.h"
#include "m_misc.h"
#include "n_network.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_colormap.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_gldefs.h"
#include "r_modes.h"
#include "r_units.h"

EDGE_DEFINE_CONSOLE_VARIABLE(field_of_view, "90", kConsoleVariableFlagArchive)

extern unsigned int root_node;

int view_window_x;
int view_window_y;
int view_window_width;
int view_window_height;

BAMAngle view_angle          = 0;
BAMAngle view_vertical_angle = 0;

vec3s view_forward;
vec3s view_up;
vec3s view_right;

BAMAngle normal_field_of_view, zoomed_field_of_view;
bool     view_is_zoomed = false;

// increment every time a check is made
int valid_count = 1;

// just for profiling purposes
int render_frame_count;
int line_count;

Subsector        *view_subsector;
RegionProperties *view_properties;

float view_x;
float view_y;
float view_z;

float view_cosine;
float view_sine;

Player *view_player;

//
// precalculated math tables
//

int reduce_flash = 0;

float sine_table[kSineTableSize];

void FreeBSP(void);

//
// To get a global angle from cartesian coordinates,
// the coordinates are flipped until they are in
// the first octant of the coordinate system, then
// the y (<=x) is scaled and divided by x to get a
// tangent (slope) value which is looked up in the
// tantoangle[] table.

static float ApproximateAtan2(float y, float x)
{
    // http://pubs.opengroup.org/onlinepubs/009695399/functions/atan2.html
    // Volkan SALMA
    const float THRQTR_PI = 3.0 * GLM_PI_4f;
    float       r, angle;
    float       abs_y = fabs(y) + 1e-10f; // kludge to prevent 0/0 condition
    if (x < 0.0f)
    {
        r     = (x + abs_y) / (abs_y - x);
        angle = THRQTR_PI;
    }
    else
    {
        r     = (x - abs_y) / (x + abs_y);
        angle = GLM_PI_4f;
    }
    angle += (0.1963f * r * r - 0.9817f) * r;
    if (y < 0.0f)
        return (-angle); // negate if in quad III or IV
    else
        return (angle);
}
//
BAMAngle PointToAngle(float x1, float y1, float x, float y)
{
    x -= x1;
    y -= y1;

    return epi::BAMFromDegrees(ApproximateAtan2(y, x) * (180.0f / GLM_PIf));
}

float PointToDistance(float x1, float y1, float x2, float y2)
{
    BAMAngle angle;
    float    dx;
    float    dy;
    float    temp;
    float    dist;

    dx = (float)fabs(x2 - x1);
    dy = (float)fabs(y2 - y1);

    if (AlmostEquals(dx, 0.0f))
        return dy;
    else if (AlmostEquals(dy, 0.0f))
        return dx;

    if (dy > dx)
    {
        temp = dx;
        dx   = dy;
        dy   = temp;
    }

    angle = epi::BAMFromATan(dy / dx) + kBAMAngle90;

    // use as cosine
    dist = dx / epi::BAMSin(angle);

    return dist;
}

//
// Called once at startup, to initialise some rendering stuff.
//
void RendererStartup(void)
{
    if (language["RefreshDaemon"])
        LogPrint("%s", language["RefreshDaemon"]);
    else
        LogPrint("Unknown Refresh Daemon");

    for (int i = 0; i < kSineTableSize; i++)
    {
        float deg = i * 360.0f / ((float)(kSineTableMask));
        glm_make_rad(&deg);
        sine_table[i] = sinf(deg);
    }

    render_frame_count = 0;
}

//
// Called at shutdown
//
void RendererShutdown(void)
{
    FreeBSP();
}

Subsector *PointInSubsector(float x, float y)
{
    BspNode     *node;
    int          side;
    unsigned int nodenum;

    nodenum = root_node;

    while (!(nodenum & kLeafSubsector))
    {
        node    = &level_nodes[nodenum];
        side    = PointOnDividingLineSide(x, y, &node->divider);
        nodenum = node->children[side];
    }

    return &level_subsectors[nodenum & ~kLeafSubsector];
}

RegionProperties *GetPointProperties(Subsector *sub, float z)
{
    // Review if vert slopes matter here - Dasho
    return sub->sector->active_properties;
}

RegionProperties *GetViewPointProperties(Subsector *sub, float z)
{
    // Review if vert slopes matter here - Dasho
    if (sub->sector->has_deep_water && z < sub->sector->deep_water_height)
        return &sub->sector->deep_water_properties;
    else
        return sub->sector->active_properties;
}

//----------------------------------------------------------------------------

// large buffers for cache coherency vs allocating each on heap
static constexpr uint16_t kMaximumDrawThings     = 32768;
static constexpr uint16_t kMaximumDrawFloors     = 32768;
static constexpr uint32_t kMaximumDrawSegs       = 65536;
static constexpr uint32_t kMaximumDrawSubsectors = 65536;

static std::vector<DrawThing>     draw_things;
static std::vector<DrawFloor>     draw_floors;
static std::vector<DrawSeg>       draw_segs;
static std::vector<DrawSubsector> draw_subsectors;

static int draw_thing_position;
static int draw_floor_position;
static int draw_seg_position;
static int draw_subsector_position;

//
// AllocateDrawStructs
//
// One-time initialisation routine.
//
void AllocateDrawStructs(void)
{
    draw_things.resize(kMaximumDrawThings);
    draw_floors.resize(kMaximumDrawFloors);
    draw_segs.resize(kMaximumDrawSegs);
    draw_subsectors.resize(kMaximumDrawSubsectors);
}

// bsp clear function

void ClearBSP(void)
{
    draw_thing_position     = 0;
    draw_floor_position     = 0;
    draw_seg_position       = 0;
    draw_subsector_position = 0;
}

void FreeBSP(void)
{
    draw_things.clear();
    draw_floors.clear();
    draw_segs.clear();
    draw_subsectors.clear();

    ClearBSP();
}

DrawThing *GetDrawThing()
{
    if (draw_thing_position >= kMaximumDrawThings)
    {
        FatalError("Max Draw Things Exceeded");
    }

    return &draw_things[draw_thing_position++];
}

DrawFloor *GetDrawFloor()
{
    if (draw_floor_position >= kMaximumDrawFloors)
    {
        FatalError("Max Draw Floors Exceeded");
    }

    return &draw_floors[draw_floor_position++];
}

DrawSeg *GetDrawSeg()
{
    if (draw_seg_position >= kMaximumDrawSegs)
    {
        FatalError("Max Draw Segs Exceeded");
    }

    return &draw_segs[draw_seg_position++];
}

DrawSubsector *GetDrawSub()
{
    if (draw_subsector_position >= kMaximumDrawSubsectors)
    {
        FatalError("Max Draw Subs Exceeded");
    }

    return &draw_subsectors[draw_subsector_position++];
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
