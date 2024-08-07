//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (BSP Traversal)
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

#include <math.h>

#include <unordered_map>
#include <unordered_set>

#include "AlmostEquals.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "edge_profiling.h"
#include "epi.h"
#include "g_game.h"
#include "i_defs_gl.h"
#include "m_bbox.h"
#include "n_network.h" // NetworkUpdate
#include "p_local.h"
#include "r_colormap.h"
#include "r_defs.h"
#include "r_effects.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_occlude.h"
#include "r_shader.h"
#include "r_sky.h"
#include "r_things.h"
#include "r_units.h"
#include "sokol_color.h"

static constexpr float kDoomYSlope     = 0.525f;
static constexpr float kDoomYSlopeFull = 0.625f;

static constexpr float kWavetableIncrement = 0.0009765625f;

static constexpr uint8_t kMaximumPolygonVertices = 64;

EDGE_DEFINE_CONSOLE_VARIABLE(debug_hall_of_mirrors, "0", kConsoleVariableFlagCheat)
EDGE_DEFINE_CONSOLE_VARIABLE(force_flat_lighting, "0", kConsoleVariableFlagArchive)

extern ConsoleVariable double_framerate;

static Sector *front_sector;
static Sector *back_sector;

unsigned int root_node;

float view_x_slope;
float view_y_slope;

static float wave_now;    // value for doing wave table lookups
static float plane_z_bob; // for floor/ceiling bob DDFSECT stuff

// -ES- 1999/03/20 Different right & left side clip angles, for asymmetric FOVs.
BAMAngle clip_left, clip_right;
BAMAngle clip_scope;

MapObject *view_camera_map_object;

float widescreen_view_width_multiplier;

static int check_coordinates[12][4] = {{kBoundingBoxRight, kBoundingBoxTop, kBoundingBoxLeft, kBoundingBoxBottom},
                                       {kBoundingBoxRight, kBoundingBoxTop, kBoundingBoxLeft, kBoundingBoxTop},
                                       {kBoundingBoxRight, kBoundingBoxBottom, kBoundingBoxLeft, kBoundingBoxTop},
                                       {0},
                                       {kBoundingBoxLeft, kBoundingBoxTop, kBoundingBoxLeft, kBoundingBoxBottom},
                                       {0},
                                       {kBoundingBoxRight, kBoundingBoxBottom, kBoundingBoxRight, kBoundingBoxTop},
                                       {0},
                                       {kBoundingBoxLeft, kBoundingBoxTop, kBoundingBoxRight, kBoundingBoxBottom},
                                       {kBoundingBoxLeft, kBoundingBoxBottom, kBoundingBoxRight, kBoundingBoxBottom},
                                       {kBoundingBoxLeft, kBoundingBoxBottom, kBoundingBoxRight, kBoundingBoxTop}};

extern float sprite_skew;

// common stuff

static Subsector *current_subsector;
static Seg       *current_seg;

static bool solid_mode;

static std::list<DrawSubsector *> draw_subsector_list;

static std::unordered_map<const Image *, GLuint> frame_texture_ids;

static GLuint R_ImageCache(const Image *image, bool anim = true, const Colormap *trans = nullptr)
{
    // (need to load the image to know the opacity)
    auto frameid = frame_texture_ids.find(image);
    if (frameid == frame_texture_ids.end())
    {
        GLuint tex_id = ImageCache(image, true, render_view_effect_colormap);
        frame_texture_ids.emplace(image, tex_id);
        return tex_id;
    }
    else
    {
        return frameid->second;
    }
}

struct WallCoordinateData
{
    int             v_count;
    const vec3s *vertices;

    GLuint tex_id;

    int pass;
    int blending;

    float R, G, B;
    float trans;

    DividingLine div;

    float tx0, ty0;
    float tx_mul, ty_mul;

    vec3s normal;

    bool mid_masked;
};

static void WallCoordFunc(void *d, int v_idx, vec3s *pos, float *rgb, vec2s *texc, vec3s *normal,
                          vec3s *lit_pos)
{
    const WallCoordinateData *data = (WallCoordinateData *)d;

    *pos    = data->vertices[v_idx];
    *normal = data->normal;

    rgb[0] = data->R;
    rgb[1] = data->G;
    rgb[2] = data->B;

    float along;

    if (fabs(data->div.delta_x) > fabs(data->div.delta_y))
    {
        along = (pos->x - data->div.x) / data->div.delta_x;
    }
    else
    {
        along = (pos->y - data->div.y) / data->div.delta_y;
    }

    texc->x = data->tx0 + along * data->tx_mul;
    texc->y = data->ty0 + pos->z * data->ty_mul;

    *lit_pos = *pos;
}

struct PlaneCoordinateData
{
    int             v_count;
    const vec3s *vertices;

    GLuint tex_id;

    int pass;
    int blending;

    float R, G, B;
    float trans;

    float tx0, ty0;
    float image_w, image_h;

    vec2s x_mat;
    vec2s y_mat;

    vec3s normal;

    // multiplier for plane_z_bob
    float bob_amount = 0;

    BAMAngle rotation = 0;
};

static void PlaneCoordFunc(void *d, int v_idx, vec3s *pos, float *rgb, vec2s *texc, vec3s *normal,
                           vec3s *lit_pos)
{
    PlaneCoordinateData *data = (PlaneCoordinateData *)d;

    *pos    = data->vertices[v_idx];
    *normal = data->normal;

    rgb[0] = data->R;
    rgb[1] = data->G;
    rgb[2] = data->B;

    vec2s rxy = {{(data->tx0 + pos->x), (data->ty0 + pos->y)}};

    if (data->rotation)
        rxy = glms_vec2_rotate(rxy, epi::RadiansFromBAM(data->rotation));

    rxy.x /= data->image_w;
    rxy.y /= data->image_h;

    texc->x = rxy.x * data->x_mat.x + rxy.y * data->x_mat.y;
    texc->y = rxy.x * data->y_mat.x + rxy.y * data->y_mat.y;

    if (data->bob_amount > 0)
        pos->z += (plane_z_bob * data->bob_amount);

    *lit_pos = *pos;
}

static void DLIT_Wall(MapObject *mo, void *dataptr)
{
    WallCoordinateData *data = (WallCoordinateData *)dataptr;

    // light behind the plane ?
    if (!mo->info_->dlight_[0].leaky_ && !data->mid_masked &&
        !(mo->subsector_->sector->floor_vertex_slope || mo->subsector_->sector->ceiling_vertex_slope))
    {
        float mx = mo->x;
        float my = mo->y;

        float dist = (mx - data->div.x) * data->div.delta_y - (my - data->div.y) * data->div.delta_x;

        if (dist < 0)
            return;
    }

    EPI_ASSERT(mo->dynamic_light_.shader);

    int blending = (data->blending & ~kBlendingAlpha) | kBlendingAdd;

    mo->dynamic_light_.shader->WorldMix(GL_POLYGON, data->v_count, data->tex_id, data->trans, &data->pass, blending,
                                        data->mid_masked, data, WallCoordFunc);
}

static void GLOWLIT_Wall(MapObject *mo, void *dataptr)
{
    WallCoordinateData *data = (WallCoordinateData *)dataptr;

    EPI_ASSERT(mo->dynamic_light_.shader);

    int blending = (data->blending & ~kBlendingAlpha) | kBlendingAdd;

    mo->dynamic_light_.shader->WorldMix(GL_POLYGON, data->v_count, data->tex_id, data->trans, &data->pass, blending,
                                        data->mid_masked, data, WallCoordFunc);
}

static void DLIT_Plane(MapObject *mo, void *dataptr)
{
    PlaneCoordinateData *data = (PlaneCoordinateData *)dataptr;

    // light behind the plane ?
    if (!mo->info_->dlight_[0].leaky_ &&
        !(mo->subsector_->sector->floor_vertex_slope || mo->subsector_->sector->ceiling_vertex_slope))
    {
        float z = data->vertices[0].z;

        if ((MapObjectMidZ(mo) > z) != (data->normal.z > 0))
            return;
    }

    // NOTE: distance already checked in DynamicLightIterator

    EPI_ASSERT(mo->dynamic_light_.shader);

    int blending = (data->blending & ~kBlendingAlpha) | kBlendingAdd;

    mo->dynamic_light_.shader->WorldMix(GL_POLYGON, data->v_count, data->tex_id, data->trans, &data->pass, blending,
                                        false /* masked */, data, PlaneCoordFunc);
}

static void GLOWLIT_Plane(MapObject *mo, void *dataptr)
{
    PlaneCoordinateData *data = (PlaneCoordinateData *)dataptr;

    EPI_ASSERT(mo->dynamic_light_.shader);

    int blending = (data->blending & ~kBlendingAlpha) | kBlendingAdd;

    mo->dynamic_light_.shader->WorldMix(GL_POLYGON, data->v_count, data->tex_id, data->trans, &data->pass, blending,
                                        false, data, PlaneCoordFunc);
}

static constexpr uint8_t kMaximumEdgeVertices = 20;

static inline void GreetNeighbourSector(float *hts, int &num, VertexSectorList *seclist)
{
    if (!seclist)
        return;

    for (int k = 0; k < (seclist->total * 2); k++)
    {
        Sector *sec = level_sectors + seclist->sectors[k / 2];

        float h = (k & 1) ? sec->ceiling_height : sec->floor_height;

        // does not intersect current height range?
        if (h <= hts[0] + 0.1 || h >= hts[num - 1] - 0.1)
            continue;

        // check if new height already present, and at same time
        // find place to insert new height.

        int pos;

        for (pos = 1; pos < num; pos++)
        {
            if (h < hts[pos] - 0.1)
                break;

            if (h < hts[pos] + 0.1)
            {
                pos = -1; // already present
                break;
            }
        }

        if (pos > 0 && pos < num)
        {
            for (int i = num; i > pos; i--)
                hts[i] = hts[i - 1];

            hts[pos] = h;

            num++;

            if (num >= kMaximumEdgeVertices)
                return;
        }
    }
}

enum WallTileFlag
{
    kWallTileIsExtra = (1 << 0),
    kWallTileExtraX  = (1 << 1), // side of an extrafloor
    kWallTileExtraY  = (1 << 2), //
    kWallTileMidMask = (1 << 4), // the mid-masked part (gratings etc)
};

static void DrawWallPart(DrawFloor *dfloor, float x1, float y1, float lz1, float lz2, float x2, float y2, float rz1,
                         float rz2, float tex_top_h, MapSurface *surf, const Image *image, bool mid_masked, bool opaque,
                         float tex_x1, float tex_x2, RegionProperties *props = nullptr)
{
    // Note: tex_x1 and tex_x2 are in world coordinates.
    //       top, bottom and tex_top_h as well.

    ec_frame_stats.draw_wall_parts++;

    (void)opaque;

    // if (! props)
    //	props = surf->override_properties ? surf->override_properties :
    // dfloor->properties;
    if (surf->override_properties)
        props = surf->override_properties;

    if (!props)
        props = dfloor->properties;

    float trans = surf->translucency;

    EPI_ASSERT(image);

    // (need to load the image to know the opacity)
    GLuint tex_id = R_ImageCache(image, true, render_view_effect_colormap);

    // ignore non-solid walls in solid mode (& vice versa)
    if ((trans < 0.99f || image->opacity_ >= kOpacityMasked) == solid_mode)
        return;

    float v_bbox[4];

    BoundingBoxClear(v_bbox);
    BoundingBoxAddPoint(v_bbox, x1, y1);
    BoundingBoxAddPoint(v_bbox, x2, y2);

    EPI_ASSERT(current_map);

    int lit_adjust = 0;

    float total_w = image->ScaledWidthTotal();
    float total_h = image->ScaledHeightTotal();

    /* convert tex_x1 and tex_x2 from world coords to texture coords */
    tex_x1 = (tex_x1 * surf->x_matrix.x) / total_w;
    tex_x2 = (tex_x2 * surf->x_matrix.x) / total_w;

    float tx0    = tex_x1;
    float tx_mul = tex_x2 - tex_x1;

    float ty_mul = surf->y_matrix.y / total_h;
    float ty0    = image->Top() - tex_top_h * ty_mul;

#if (DEBUG >= 3)
    LogDebug("WALL (%d,%d,%d) -> (%d,%d,%d)\n", (int)x1, (int)y1, (int)top, (int)x2, (int)y2, (int)bottom);
#endif

    // -AJA- 2007/08/07: ugly code here ensures polygon edges
    //       match up with adjacent linedefs (otherwise small
    //       gaps can appear which look bad).

    float left_h[kMaximumEdgeVertices];
    int   left_num = 2;
    float right_h[kMaximumEdgeVertices];
    int   right_num = 2;

    left_h[0]  = lz1;
    left_h[1]  = lz2;
    right_h[0] = rz1;
    right_h[1] = rz2;

    if (solid_mode && !mid_masked)
    {
        GreetNeighbourSector(left_h, left_num, current_seg->vertex_sectors[0]);
        GreetNeighbourSector(right_h, right_num, current_seg->vertex_sectors[1]);
    }

    vec3s vertices[kMaximumEdgeVertices * 2];

    int v_count = 0;

    for (int LI = 0; LI < left_num; LI++)
    {
        vertices[v_count].x = x1;
        vertices[v_count].y = y1;
        vertices[v_count].z = left_h[LI];

        v_count++;
    }

    for (int RI = right_num - 1; RI >= 0; RI--)
    {
        vertices[v_count].x = x2;
        vertices[v_count].y = y2;
        vertices[v_count].z = right_h[RI];

        v_count++;
    }

    int blending;

    if (trans >= 0.99f && image->opacity_ == kOpacitySolid)
        blending = kBlendingNone;
    else if (trans < 0.11f || image->opacity_ == kOpacityComplex)
        blending = kBlendingMasked;
    else
        blending = kBlendingLess;

    if (trans < 0.99f || image->opacity_ == kOpacityComplex)
        blending |= kBlendingAlpha;

    // -AJA- 2006-06-22: fix for midmask wrapping bug
    if (mid_masked &&
        (!current_seg->linedef->special || AlmostEquals(current_seg->linedef->special->s_yspeed_,
                                                        0.0f))) // Allow vertical scroller midmasks
        blending |= kBlendingClampY;

    WallCoordinateData data;

    data.v_count  = v_count;
    data.vertices = vertices;

    data.R = data.G = data.B = 1.0f;

    data.div.x       = x1;
    data.div.y       = y1;
    data.div.delta_x = x2 - x1;
    data.div.delta_y = y2 - y1;

    data.tx0    = tx0;
    data.ty0    = ty0;
    data.tx_mul = tx_mul;
    data.ty_mul = ty_mul;

    // TODO: make a unit vector
    data.normal = {{(y2 - y1), (x1 - x2), 0}};

    data.tex_id     = tex_id;
    data.pass       = 0;
    data.blending   = blending;
    data.trans      = trans;
    data.mid_masked = mid_masked;

    AbstractShader *cmap_shader = GetColormapShader(props, lit_adjust, current_subsector->sector);

    cmap_shader->WorldMix(GL_POLYGON, data.v_count, data.tex_id, trans, &data.pass, data.blending, data.mid_masked,
                          &data, WallCoordFunc);

    if (render_view_extra_light < 250)
    {
        float bottom = GLM_MIN(lz1, rz1);
        float top    = GLM_MAX(lz2, rz2);

        DynamicLightIterator(v_bbox[kBoundingBoxLeft], v_bbox[kBoundingBoxBottom], bottom, v_bbox[kBoundingBoxRight],
                             v_bbox[kBoundingBoxTop], top, DLIT_Wall, &data);

        SectorGlowIterator(current_seg->front_sector, v_bbox[kBoundingBoxLeft], v_bbox[kBoundingBoxBottom], bottom,
                           v_bbox[kBoundingBoxRight], v_bbox[kBoundingBoxTop], top, GLOWLIT_Wall, &data);
    }
}

static void DrawSlidingDoor(DrawFloor *dfloor, float c, float f, float tex_top_h, MapSurface *surf, bool opaque,
                            float x_offset)
{
    /* smov may be nullptr */
    SlidingDoorMover *smov = current_seg->linedef->slider_move;

    float opening = smov ? smov->opening : 0;

    Line *ld = current_seg->linedef;

    /// float im_width = wt->surface->image->ScaledWidthActual();

    int num_parts = 1;
    if (current_seg->linedef->slide_door->s_.type_ == kSlidingDoorTypeCenter)
        num_parts = 2;

    // extent of current seg along the linedef
    float s_seg, e_seg;

    if (current_seg->side == 0)
    {
        s_seg = current_seg->offset;
        e_seg = s_seg + current_seg->length;
    }
    else
    {
        e_seg = ld->length - current_seg->offset;
        s_seg = e_seg - current_seg->length;
    }

    for (int part = 0; part < num_parts; part++)
    {
        // coordinates along the linedef (0.00 at V1, 1.00 at V2)
        float s_along, s_tex;
        float e_along, e_tex;

        switch (current_seg->linedef->slide_door->s_.type_)
        {
        case kSlidingDoorTypeLeft:
            s_along = 0;
            e_along = ld->length - opening;

            s_tex = -e_along;
            e_tex = 0;
            break;

        case kSlidingDoorTypeRight:
            s_along = opening;
            e_along = ld->length;

            s_tex = 0;
            e_tex = e_along - s_along;
            break;

        case kSlidingDoorTypeCenter:
            if (part == 0)
            {
                s_along = 0;
                e_along = (ld->length - opening) / 2;

                e_tex = ld->length / 2;
                s_tex = e_tex - (e_along - s_along);
            }
            else
            {
                s_along = (ld->length + opening) / 2;
                e_along = ld->length;

                s_tex = ld->length / 2;
                e_tex = s_tex + (e_along - s_along);
            }
            break;

        default:
            FatalError("INTERNAL ERROR: unknown slidemove type!\n");
            return; /* NOT REACHED */
        }

        // limit sliding door coordinates to current seg
        if (s_along < s_seg)
        {
            s_tex += (s_seg - s_along);
            s_along = s_seg;
        }
        if (e_along > e_seg)
        {
            e_tex += (e_seg - e_along);
            e_along = e_seg;
        }

        if (s_along >= e_along)
            continue;

        float x1 = ld->vertex_1->x + ld->delta_x * s_along / ld->length;
        float y1 = ld->vertex_1->y + ld->delta_y * s_along / ld->length;

        float x2 = ld->vertex_1->x + ld->delta_x * e_along / ld->length;
        float y2 = ld->vertex_1->y + ld->delta_y * e_along / ld->length;

        s_tex += x_offset;
        e_tex += x_offset;

        DrawWallPart(dfloor, x1, y1, f, c, x2, y2, f, c, tex_top_h, surf, surf->image, true, opaque, s_tex, e_tex);
    }
}

// Mirror the texture on the back of the line
static void DrawGlass(DrawFloor *dfloor, float c, float f, float tex_top_h, MapSurface *surf, bool opaque,
                      float x_offset)
{
    Line *ld = current_seg->linedef;

    // extent of current seg along the linedef
    float s_seg, e_seg;

    if (current_seg->side == 0)
    {
        s_seg = current_seg->offset;
        e_seg = s_seg + current_seg->length;
    }
    else
    {
        e_seg = ld->length - current_seg->offset;
        s_seg = e_seg - current_seg->length;
    }

    // coordinates along the linedef (0.00 at V1, 1.00 at V2)
    float s_along, s_tex;
    float e_along, e_tex;

    s_along = 0;
    e_along = ld->length - 0;

    s_tex = -e_along;
    e_tex = 0;

    // limit glass coordinates to current seg
    if (s_along < s_seg)
    {
        s_tex += (s_seg - s_along);
        s_along = s_seg;
    }
    if (e_along > e_seg)
    {
        e_tex += (e_seg - e_along);
        e_along = e_seg;
    }

    if (s_along < e_along)
    {
        float x1 = ld->vertex_1->x + ld->delta_x * s_along / ld->length;
        float y1 = ld->vertex_1->y + ld->delta_y * s_along / ld->length;

        float x2 = ld->vertex_1->x + ld->delta_x * e_along / ld->length;
        float y2 = ld->vertex_1->y + ld->delta_y * e_along / ld->length;

        s_tex += x_offset;
        e_tex += x_offset;

        DrawWallPart(dfloor, x1, y1, f, c, x2, y2, f, c, tex_top_h, surf, surf->image, true, opaque, s_tex, e_tex);
    }
}

static void DrawTile(Seg *seg, DrawFloor *dfloor, float lz1, float lz2, float rz1, float rz2, float tex_z, int flags,
                     MapSurface *surf)
{
    EDGE_ZoneScoped;

    // tex_z = texturing top, in world coordinates

    const Image *image = surf->image;

    if (!image)
        image = ImageForHomDetect();

    float tex_top_h = tex_z + surf->offset.y;
    float x_offset  = surf->offset.x;

    if (flags & kWallTileExtraX)
    {
        x_offset += seg->sidedef->middle.offset.x;
    }
    if (flags & kWallTileExtraY)
    {
        // needed separate Y flag to maintain compatibility
        tex_top_h += seg->sidedef->middle.offset.y;
    }

    bool opaque = (!seg->back_sector) || (surf->translucency >= 0.99f && image->opacity_ == kOpacitySolid);

    // check for horizontal sliders
    if ((flags & kWallTileMidMask) && seg->linedef->slide_door)
    {
        if (surf->image)
            DrawSlidingDoor(dfloor, lz2, lz1, tex_top_h, surf, opaque, x_offset);
        return;
    }

    // check for breakable glass
    if (seg->linedef->special)
    {
        if ((flags & kWallTileMidMask) && seg->linedef->special->glass_)
        {
            if (surf->image)
                DrawGlass(dfloor, lz2, lz1, tex_top_h, surf, opaque, x_offset);
            return;
        }
    }

    float x1 = seg->vertex_1->x;
    float y1 = seg->vertex_1->y;
    float x2 = seg->vertex_2->x;
    float y2 = seg->vertex_2->y;

    float tex_x1 = seg->offset;
    float tex_x2 = tex_x1 + seg->length;

    tex_x1 += x_offset;
    tex_x2 += x_offset;

    if (seg->sidedef->sector->properties.special && seg->sidedef->sector->properties.special->floor_bob_ > 0)
    {
        lz1 -= seg->sidedef->sector->properties.special->floor_bob_;
        rz1 -= seg->sidedef->sector->properties.special->floor_bob_;
    }

    if (seg->sidedef->sector->properties.special && seg->sidedef->sector->properties.special->ceiling_bob_ > 0)
    {
        lz2 += seg->sidedef->sector->properties.special->ceiling_bob_;
        rz2 += seg->sidedef->sector->properties.special->ceiling_bob_;
    }

    DrawWallPart(dfloor, x1, y1, lz1, lz2, x2, y2, rz1, rz2, tex_top_h, surf, image,
                 (flags & kWallTileMidMask) ? true : false, opaque, tex_x1, tex_x2,
                 (flags & kWallTileMidMask) ? &seg->sidedef->sector->properties : nullptr);
}

static inline void AddWallTile(Seg *seg, DrawFloor *dfloor, MapSurface *surf, float z1, float z2, float tex_z,
                               int flags, float f_min, float c_max)
{
    z1 = GLM_MAX(f_min, z1);
    z2 = GLM_MIN(c_max, z2);

    if (z1 >= z2 - 0.01)
        return;

    DrawTile(seg, dfloor, z1, z2, z1, z2, tex_z, flags, surf);
}

static inline void AddWallTile2(Seg *seg, DrawFloor *dfloor, MapSurface *surf, float lz1, float lz2, float rz1,
                                float rz2, float tex_z, int flags)
{
    DrawTile(seg, dfloor, lz1, lz2, rz1, rz2, tex_z, flags, surf);
}

static inline float SafeImageHeight(const Image *image)
{
    if (image)
        return image->ScaledHeightActual();
    else
        return 0;
}

static void ComputeWallTiles(Seg *seg, DrawFloor *dfloor, int sidenum, float f_min, float c_max)
{
    EDGE_ZoneScoped;

    Line       *ld = seg->linedef;
    Side       *sd = ld->side[sidenum];
    Sector     *sec, *other;

    float       tex_z;

    bool lower_invis = false;
    bool upper_invis = false;

    if (!sd)
        return;

    sec   = sd->sector;
    other = sidenum ? ld->front_sector : ld->back_sector;

    float slope_fh = sec->floor_height;

    float slope_ch = sec->ceiling_height;

    /*RGBAColor sec_fc = sec->properties.fog_color;
    float     sec_fd = sec->properties.fog_density;
    // check for DDFLEVL fog
    if (sec_fc == kRGBANoValue)
    {
        if (EDGE_IMAGE_IS_SKY(seg->sidedef->sector->ceiling))
        {
            sec_fc = current_map->outdoor_fog_color_;
            sec_fd = 0.01f * current_map->outdoor_fog_density_;
        }
        else
        {
            sec_fc = current_map->indoor_fog_color_;
            sec_fd = 0.01f * current_map->indoor_fog_density_;
        }
    }
    RGBAColor other_fc = (other ? other->properties.fog_color : kRGBANoValue);
    float     other_fd = (other ? other->properties.fog_density : 0.0f);
    if (other_fc == kRGBANoValue)
    {
        if (other)
        {
            if (EDGE_IMAGE_IS_SKY(other->ceiling))
            {
                other_fc = current_map->outdoor_fog_color_;
                other_fd = 0.01f * current_map->outdoor_fog_density_;
            }
            else
            {
                other_fc = current_map->indoor_fog_color_;
                other_fd = 0.01f * current_map->indoor_fog_density_;
            }
        }
    }*/

    if (!other)
    {
        if (!sd->middle.image && !debug_hall_of_mirrors.d_)
            return;

        AddWallTile(seg, dfloor, &sd->middle, slope_fh, slope_ch,
                    (ld->flags & kLineFlagLowerUnpegged)
                        ? sec->floor_height + (SafeImageHeight(sd->middle.image) / sd->middle.y_matrix.y)
                        : sec->ceiling_height,
                    0, f_min, c_max);
        return;
    }

    // handle lower, upper and mid-masker

    if (slope_fh < other->floor_height || (sec->floor_vertex_slope || other->floor_vertex_slope))
    {
        if (!sec->floor_vertex_slope && other->floor_vertex_slope)
        {
            float zv1 = seg->vertex_1->z;
            float zv2 = seg->vertex_2->z;
            AddWallTile2(seg, dfloor, sd->bottom.image ? &sd->bottom : &other->floor, sec->floor_height,
                         (zv1 < 32767.0f && zv1 > -32768.0f) ? zv1 : sec->floor_height, sec->floor_height,
                         (zv2 < 32767.0f && zv2 > -32768.0f) ? zv2 : sec->floor_height,
                         (ld->flags & kLineFlagLowerUnpegged) ? sec->ceiling_height
                                                              : GLM_MAX(sec->floor_height, GLM_MAX(zv1, zv2)),
                         0);
        }
        else if (sec->floor_vertex_slope && !other->floor_vertex_slope)
        {
            float zv1 = seg->vertex_1->z;
            float zv2 = seg->vertex_2->z;
            AddWallTile2(seg, dfloor, sd->bottom.image ? &sd->bottom : &sec->floor,
                         (zv1 < 32767.0f && zv1 > -32768.0f) ? zv1 : other->floor_height, other->floor_height,
                         (zv2 < 32767.0f && zv2 > -32768.0f) ? zv2 : other->floor_height, other->floor_height,
                         (ld->flags & kLineFlagLowerUnpegged) ? other->ceiling_height
                                                              : GLM_MAX(other->floor_height, GLM_MAX(zv1, zv2)),
                         0);
        }
        else if (!sd->bottom.image && !debug_hall_of_mirrors.d_)
        {
            lower_invis = true;
        }
        else
        {
            AddWallTile(seg, dfloor, &sd->bottom, slope_fh, other->floor_height,
                        (ld->flags & kLineFlagLowerUnpegged) ? sec->ceiling_height : other->floor_height, 0, f_min,
                        c_max);
        }
    }

    if ((slope_ch > other->ceiling_height || (sec->ceiling_vertex_slope || other->ceiling_vertex_slope)) &&
        !(EDGE_IMAGE_IS_SKY(sec->ceiling) && EDGE_IMAGE_IS_SKY(other->ceiling)))
    {
        if (!sec->ceiling_vertex_slope && other->ceiling_vertex_slope)
        {
            float zv1 = seg->vertex_1->w;
            float zv2 = seg->vertex_2->w;
            AddWallTile2(seg, dfloor, sd->top.image ? &sd->top : &other->ceiling, sec->ceiling_height,
                         (zv1 < 32767.0f && zv1 > -32768.0f) ? zv1 : sec->ceiling_height, sec->ceiling_height,
                         (zv2 < 32767.0f && zv2 > -32768.0f) ? zv2 : sec->ceiling_height,
                         (ld->flags & kLineFlagUpperUnpegged) ? sec->floor_height : GLM_MIN(zv1, zv2), 0);
        }
        else if (sec->ceiling_vertex_slope && !other->ceiling_vertex_slope)
        {
            float zv1 = seg->vertex_1->w;
            float zv2 = seg->vertex_2->w;
            AddWallTile2(seg, dfloor, sd->top.image ? &sd->top : &sec->ceiling, other->ceiling_height,
                         (zv1 < 32767.0f && zv1 > -32768.0f) ? zv1 : other->ceiling_height, other->ceiling_height,
                         (zv2 < 32767.0f && zv2 > -32768.0f) ? zv2 : other->ceiling_height,
                         (ld->flags & kLineFlagUpperUnpegged) ? other->floor_height : GLM_MIN(zv1, zv2), 0);
        }
        else if (!sd->top.image && !debug_hall_of_mirrors.d_)
        {
            upper_invis = true;
        }
        else
        {
            AddWallTile(seg, dfloor, &sd->top, other->ceiling_height, slope_ch,
                        (ld->flags & kLineFlagUpperUnpegged) ? sec->ceiling_height
                                                             : other->ceiling_height + SafeImageHeight(sd->top.image),
                        0, f_min, c_max);
        }
    }

    if (sd->middle.image)
    {
        float f1 = GLM_MAX(sec->floor_height, other->floor_height);
        float c1 = GLM_MIN(sec->ceiling_height, other->ceiling_height);

        float f2, c2;

        if (ld->flags & kLineFlagLowerUnpegged)
        {
            f2 = f1 + sd->middle_mask_offset;
            c2 = f2 + (sd->middle.image->ScaledHeightActual() / sd->middle.y_matrix.y);
        }
        else
        {
            c2 = c1 + sd->middle_mask_offset;
            f2 = c2 - (sd->middle.image->ScaledHeightActual() / sd->middle.y_matrix.y);
        }

        tex_z = c2;

        // hack for transparent doors
        {
            if (lower_invis)
                f1 = sec->floor_height;
            if (upper_invis)
                c1 = sec->ceiling_height;
        }

        // hack for "see-through" lines (same sector on both sides)
        if (sec != other)
        {
            f2 = GLM_MAX(f2, f1);
            c2 = GLM_MIN(c2, c1);
        }

        if (c2 > f2)
        {
            AddWallTile(seg, dfloor, &sd->middle, f2, c2, tex_z, kWallTileMidMask, f_min, c_max);
        }
    }
}

static void RenderSeg(DrawFloor *dfloor, Seg *seg)
{
    //
    // Analyses floor/ceiling heights, and add corresponding walls/floors
    // to the drawfloor.  Returns true if the whole region was "solid".
    //
    current_seg = seg;

    EPI_ASSERT(!seg->miniseg && seg->linedef);

    // mark the segment on the automap
    seg->linedef->flags |= kLineFlagMapped;

    front_sector = seg->front_subsector->sector;
    back_sector  = nullptr;

    if (seg->back_subsector)
        back_sector = seg->back_subsector->sector;

    float f_min = dfloor->is_lowest ? -32767.0 : dfloor->floor_height;
    float c_max = dfloor->is_highest ? +32767.0 : dfloor->ceiling_height;

#if (DEBUG >= 3)
    LogDebug("   BUILD WALLS %1.1f .. %1.1f\n", f_min, c1);
#endif

    ComputeWallTiles(seg, dfloor, seg->side, f_min, c_max);
}

static void RendererWalkBspNode(unsigned int bspnum);

//
// RendererWalkSeg
//
// Visit a single seg of the subsector, and for one-sided lines update
// the 1D occlusion buffer.
//
static void RendererWalkSeg(DrawSubsector *dsub, Seg *seg)
{
    EDGE_ZoneScoped;

    float sx1 = seg->vertex_1->x;
    float sy1 = seg->vertex_1->y;

    float sx2 = seg->vertex_2->x;
    float sy2 = seg->vertex_2->y;

    BAMAngle angle_L = PointToAngle(view_x, view_y, sx1, sy1);
    BAMAngle angle_R = PointToAngle(view_x, view_y, sx2, sy2);

    // Clip to view edges.

    BAMAngle span = angle_L - angle_R;

    // back side ?
    if (span >= kBAMAngle180)
        return;

    angle_L -= view_angle;
    angle_R -= view_angle;

    if (clip_scope != kBAMAngle180)
    {
        BAMAngle tspan1 = angle_L - clip_right;
        BAMAngle tspan2 = clip_left - angle_R;

        if (tspan1 > clip_scope)
        {
            // Totally off the left edge?
            if (tspan2 >= kBAMAngle180)
                return;

            angle_L = clip_left;
        }

        if (tspan2 > clip_scope)
        {
            // Totally off the left edge?
            if (tspan1 >= kBAMAngle180)
                return;

            angle_R = clip_right;
        }

        span = angle_L - angle_R;
    }

    // The seg is in the view range,
    // but not necessarily visible.

    // check if visible
    if (span > (kBAMAngle1 / 4) && OcclusionTest(angle_R, angle_L))
    {
        return;
    }

    dsub->visible = true;

    if (seg->miniseg || span == 0)
        return;

    DrawSeg *dseg = GetDrawSeg();
    dseg->seg     = seg;

    dsub->segs.push_back(dseg);

    Sector *fsector = seg->front_subsector->sector;
    Sector *bsector = nullptr;

    if (seg->back_subsector)
        bsector = seg->back_subsector->sector;

    // only 1 sided walls affect the 1D occlusion buffer

    if (seg->linedef->blocked)
        OcclusionSet(angle_R, angle_L);

    // --- handle sky (using the depth buffer) ---

    if (bsector && EDGE_IMAGE_IS_SKY(fsector->floor) && EDGE_IMAGE_IS_SKY(bsector->floor))
    {
        if (fsector->floor_height < bsector->floor_height)
        {
            RenderSkyWall(seg, fsector->floor_height, bsector->floor_height);
        }
    }

    if (EDGE_IMAGE_IS_SKY(fsector->ceiling))
    {
        if (fsector->ceiling_height < fsector->sky_height &&
            (!bsector || !EDGE_IMAGE_IS_SKY(bsector->ceiling) || bsector->floor_height >= fsector->ceiling_height))
        {
            RenderSkyWall(seg, fsector->ceiling_height, fsector->sky_height);
        }
        else if (bsector && EDGE_IMAGE_IS_SKY(bsector->ceiling))
        {
            float max_f = GLM_MAX(fsector->floor_height, bsector->floor_height);

            if (bsector->ceiling_height <= max_f && max_f < fsector->sky_height)
            {
                RenderSkyWall(seg, max_f, fsector->sky_height);
            }
        }
    }
    // -AJA- 2004/08/29: Emulate Sky-Flooding TRICK
    else if (!debug_hall_of_mirrors.d_ && bsector && EDGE_IMAGE_IS_SKY(bsector->ceiling) &&
             seg->sidedef->top.image == nullptr && bsector->ceiling_height < fsector->ceiling_height)
    {
        RenderSkyWall(seg, bsector->ceiling_height, fsector->ceiling_height);
    }
}

//
// RendererCheckBBox
//
// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.
//
// Placed here to be close to RendererWalkSeg(), which has similiar angle
// clipping stuff in it.
//
bool RendererCheckBBox(float *bspcoord)
{
    EDGE_ZoneScoped;

    int boxx, boxy;

    // Find the corners of the box
    // that define the edges from current viewpoint.
    if (view_x <= bspcoord[kBoundingBoxLeft])
        boxx = 0;
    else if (view_x < bspcoord[kBoundingBoxRight])
        boxx = 1;
    else
        boxx = 2;

    if (view_y >= bspcoord[kBoundingBoxTop])
        boxy = 0;
    else if (view_y > bspcoord[kBoundingBoxBottom])
        boxy = 1;
    else
        boxy = 2;

    int boxpos = (boxy << 2) + boxx;

    if (boxpos == 5)
        return true;

    float x1 = bspcoord[check_coordinates[boxpos][0]];
    float y1 = bspcoord[check_coordinates[boxpos][1]];
    float x2 = bspcoord[check_coordinates[boxpos][2]];
    float y2 = bspcoord[check_coordinates[boxpos][3]];

    // check clip list for an open space
    BAMAngle angle_L = PointToAngle(view_x, view_y, x1, y1);
    BAMAngle angle_R = PointToAngle(view_x, view_y, x2, y2);

    BAMAngle span = angle_L - angle_R;

    // Sitting on a line?
    if (span >= kBAMAngle180)
        return true;

    angle_L -= view_angle;
    angle_R -= view_angle;

    if (clip_scope != kBAMAngle180)
    {
        BAMAngle tspan1 = angle_L - clip_right;
        BAMAngle tspan2 = clip_left - angle_R;

        if (tspan1 > clip_scope)
        {
            // Totally off the left edge?
            if (tspan2 >= kBAMAngle180)
                return false;

            angle_L = clip_left;
        }

        if (tspan2 > clip_scope)
        {
            // Totally off the right edge?
            if (tspan1 >= kBAMAngle180)
                return false;

            angle_R = clip_right;
        }

        if (angle_L == angle_R)
            return false;
    }

    return !OcclusionTest(angle_R, angle_L);
}

static void RenderPlane(DrawFloor *dfloor, float h, MapSurface *surf, int face_dir)
{
    EDGE_ZoneScoped;

    int num_vert, i;

    if (!surf->image)
        return;

    // ignore sky
    if (EDGE_IMAGE_IS_SKY(*surf))
        return;

    ec_frame_stats.draw_planes++;

    RegionProperties *props = dfloor->properties;

    if (surf->override_properties)
        props = surf->override_properties;

    float trans = surf->translucency;

    // ignore invisible planes
    if (trans < 0.01f)
        return;

    // ignore non-facing planes
    if ((view_z > h) != (face_dir > 0) && !current_subsector->sector->floor_vertex_slope)
        return;

    // ignore dud regions (floor >= ceiling)
    if (dfloor->floor_height > dfloor->ceiling_height && !current_subsector->sector->ceiling_vertex_slope)
        return;

    // ignore empty subsectors
    if (current_subsector->segs == nullptr)
        return;

    // (need to load the image to know the opacity)
    GLuint tex_id = R_ImageCache(surf->image, true, render_view_effect_colormap);

    // ignore non-solid planes in solid_mode (& vice versa)
    if ((trans < 0.99f || surf->image->opacity_ >= kOpacityMasked) == solid_mode)
        return;

    // count number of actual vertices
    Seg *seg;
    for (seg = current_subsector->segs, num_vert = 0; seg; seg = seg->subsector_next, num_vert++)
    {
        /* no other code needed */
    }

    // -AJA- make sure polygon has enough vertices.  Sometimes a subsector
    // ends up with only 1 or 2 segs due to level problems (e.g. MAP22).
    if (num_vert < 3)
        return;

    if (num_vert > kMaximumPolygonVertices)
        num_vert = kMaximumPolygonVertices;

    vec3s vertices[kMaximumPolygonVertices];

    float v_bbox[4];

    BoundingBoxClear(v_bbox);

    int v_count = 0;

    for (seg = current_subsector->segs, i = 0; seg && (i < kMaximumPolygonVertices); seg = seg->subsector_next, i++)
    {
        if (v_count < kMaximumPolygonVertices)
        {
            float x = seg->vertex_1->x;
            float y = seg->vertex_1->y;
            float z = h;

            BoundingBoxAddPoint(v_bbox, x, y);

            if (current_subsector->sector->floor_vertex_slope && face_dir > 0)
            {
                // floor - check vertex heights
                if (seg->vertex_1->z < 32767.0f && seg->vertex_1->z > -32768.0f)
                    z = seg->vertex_1->z;
            }

            if (current_subsector->sector->ceiling_vertex_slope && face_dir < 0)
            {
                // ceiling - check vertex heights
                if (seg->vertex_1->w < 32767.0f && seg->vertex_1->w > -32768.0f)
                    z = seg->vertex_1->w;
            }

            vertices[v_count].x = x;
            vertices[v_count].y = y;
            vertices[v_count].z = z;

            v_count++;
        }
    }

    int blending;

    if (trans >= 0.99f && surf->image->opacity_ == kOpacitySolid)
        blending = kBlendingNone;
    else if (trans < 0.11f || surf->image->opacity_ == kOpacityComplex)
        blending = kBlendingMasked;
    else
        blending = kBlendingLess;

    if (trans < 0.99f || surf->image->opacity_ == kOpacityComplex)
        blending |= kBlendingAlpha;

    PlaneCoordinateData data;

    data.v_count  = v_count;
    data.vertices = vertices;
    data.R = data.G = data.B = 1.0f;
    data.tx0                 = surf->offset.x;
    data.ty0                 = surf->offset.y;
    data.image_w             = surf->image->ScaledWidthActual();
    data.image_h             = surf->image->ScaledHeightActual();
    data.x_mat               = surf->x_matrix;
    data.y_mat               = surf->y_matrix;
    data.normal   = {{0, 0, (view_z > h) ? 1.0f : -1.0f}};
    data.tex_id   = tex_id;
    data.pass     = 0;
    data.blending = blending;
    data.trans    = trans;
    data.rotation = surf->rotation;

    if (current_subsector->sector->properties.special)
    {
        if (face_dir > 0)
            data.bob_amount = current_subsector->sector->properties.special->floor_bob_;
        else
            data.bob_amount = current_subsector->sector->properties.special->ceiling_bob_;
    }

    AbstractShader *cmap_shader = GetColormapShader(props, 0, current_subsector->sector);

    cmap_shader->WorldMix(GL_POLYGON, data.v_count, data.tex_id, trans, &data.pass, data.blending, false /* masked */,
                          &data, PlaneCoordFunc);

    if (render_view_extra_light < 250)
    {
        DynamicLightIterator(v_bbox[kBoundingBoxLeft], v_bbox[kBoundingBoxBottom], h, v_bbox[kBoundingBoxRight],
                             v_bbox[kBoundingBoxTop], h, DLIT_Plane, &data);

        SectorGlowIterator(current_subsector->sector, v_bbox[kBoundingBoxLeft], v_bbox[kBoundingBoxBottom], h,
                           v_bbox[kBoundingBoxRight], v_bbox[kBoundingBoxTop], h, GLOWLIT_Plane, &data);
    }
}

static inline void AddNewDrawFloor(DrawSubsector *dsub, float floor_height, float ceiling_height,
                                   float top_h, MapSurface *floor, MapSurface *ceil, RegionProperties *props)
{
    DrawFloor *dfloor;

    dfloor = GetDrawFloor();

    dfloor->is_highest      = false;
    dfloor->is_lowest       = false;
    dfloor->render_next     = nullptr;
    dfloor->render_previous = nullptr;
    dfloor->floor           = nullptr;
    dfloor->ceiling         = nullptr;
    dfloor->properties      = nullptr;
    dfloor->things          = nullptr;

    dfloor->floor_height   = floor_height;
    dfloor->ceiling_height = ceiling_height;
    dfloor->top_height     = top_h;
    dfloor->floor          = floor;
    dfloor->ceiling        = ceil;
    dfloor->properties     = props;

    // link it in, height order

    dsub->floors.push_back(dfloor);

    // link it in, rendering order (very important)

    if (dsub->render_floors == nullptr || floor_height > view_z)
    {
        // add to head
        dfloor->render_next     = dsub->render_floors;
        dfloor->render_previous = nullptr;

        if (dsub->render_floors)
            dsub->render_floors->render_previous = dfloor;

        dsub->render_floors = dfloor;
    }
    else
    {
        // add to tail
        DrawFloor *tail;

        for (tail = dsub->render_floors; tail->render_next; tail = tail->render_next)
        { /* nothing here */
        }

        dfloor->render_next     = nullptr;
        dfloor->render_previous = tail;

        tail->render_next = dfloor;
    }
}

//
// RendererWalkSubsector
//
// Visit a subsector, and collect information, such as where the
// walls, planes (ceilings & floors) and things need to be drawn.
//
static void RendererWalkSubsector(int num)
{
    EDGE_ZoneScoped;

    Subsector *sub    = &level_subsectors[num];
    Sector    *sector = sub->sector;

    // store subsector in a global var for other functions to use
    current_subsector = sub;

#if (DEBUG >= 1)
    LogDebug("\nVISITING SUBSEC %d (sector %d)\n\n", num, sub->sector - level_sectors);
#endif

    DrawSubsector *K = GetDrawSub();
    K->subsector     = sub;
    K->visible       = false;
    K->sorted        = false;
    K->render_floors = nullptr;

    K->floors.clear();
    K->segs.clear();

    // --- handle sky (using the depth buffer) ---

    if (EDGE_IMAGE_IS_SKY(sub->sector->floor) && view_z > sub->sector->floor_height)
    {
        RenderSkyPlane(sub, sub->sector->floor_height);
    }

    if (EDGE_IMAGE_IS_SKY(sub->sector->ceiling) && view_z < sub->sector->sky_height)
    {
        RenderSkyPlane(sub, sub->sector->sky_height);
    }

    float floor_h = sector->floor_height;
    float ceil_h  = sector->ceiling_height;

    MapSurface *floor_s = &sector->floor;
    MapSurface *ceil_s  = &sector->ceiling;

    RegionProperties *props = sector->active_properties;

    // deep water FX
    if (sector->has_deep_water)
    {
        AddNewDrawFloor(K, floor_h, sector->deep_water_height, sector->deep_water_height, floor_s, &sector->deep_water_surface, &sector->deep_water_properties);
        AddNewDrawFloor(K, sector->deep_water_height, ceil_h, ceil_h, &sector->deep_water_surface, ceil_s, props);
    }
    else
        AddNewDrawFloor(K, floor_h, ceil_h, ceil_h, floor_s, ceil_s, props);

    K->floors[0]->is_lowest                     = true;
    K->floors[K->floors.size() - 1]->is_highest = true;

    // handle each sprite in the subsector.  Must be done before walls,
    // since the wall code will update the 1D occlusion buffer.

    for (MapObject *mo = sub->thing_list; mo; mo = mo->subsector_next_)
    {
        RendererWalkThing(K, mo);
    }
    // clip 1D occlusion buffer.
    for (Seg *seg = sub->segs; seg; seg = seg->subsector_next)
    {
        RendererWalkSeg(K, seg);
    }

    // add drawsub to list (closest -> furthest)
    draw_subsector_list.push_back(K);
}

static void RenderSubsector(DrawSubsector *dsub);

static void RenderSubList(std::list<DrawSubsector *> &dsubs)
{
    // draw all solid walls and planes
    solid_mode = true;
    StartUnitBatch(solid_mode);

    std::list<DrawSubsector *>::iterator FI; // Forward Iterator

    for (FI = dsubs.begin(); FI != dsubs.end(); FI++)
        RenderSubsector(*FI);

    FinishUnitBatch();

    // draw all sprites and masked/translucent walls/planes
    solid_mode = false;
    StartUnitBatch(solid_mode);

    std::list<DrawSubsector *>::reverse_iterator RI;

    for (RI = dsubs.rbegin(); RI != dsubs.rend(); RI++)
        RenderSubsector(*RI);

    FinishUnitBatch();
}

static void RenderSubsector(DrawSubsector *dsub)
{
    EDGE_ZoneScoped;

    Subsector *sub = dsub->subsector;

#if (DEBUG >= 1)
    LogDebug("\nREVISITING SUBSEC %d\n\n", (int)(sub - subsectors));
#endif

    current_subsector = sub;

    DrawFloor *dfloor;

    // handle each floor, drawing planes and things
    for (dfloor = dsub->render_floors; dfloor != nullptr; dfloor = dfloor->render_next)
    {
        for (std::list<DrawSeg *>::iterator iter = dsub->segs.begin(), iter_end = dsub->segs.end(); iter != iter_end;
             iter++)
        {
            RenderSeg(dfloor, (*iter)->seg);
        }

        RenderPlane(dfloor, dfloor->ceiling_height, dfloor->ceiling, -1);
        RenderPlane(dfloor, dfloor->floor_height, dfloor->floor, +1);

        if (!solid_mode)
        {
            SortRenderThings(dfloor);
        }
    }
}

//
// RendererWalkBspNode
//
// Walks all subsectors below a given node, traversing subtree
// recursively, collecting information.  Just call with BSP root.
//
static void RendererWalkBspNode(unsigned int bspnum)
{
    EDGE_ZoneScoped;

    BspNode *node;
    int      side;

    // Found a subsector?
    if (bspnum & kLeafSubsector)
    {
        RendererWalkSubsector(bspnum & (~kLeafSubsector));
        return;
    }

    node = &level_nodes[bspnum];

    // Decide which side the view point is on.

    DividingLine nd_div;

    nd_div.x       = node->divider.x;
    nd_div.y       = node->divider.y;
    nd_div.delta_x = node->divider.delta_x;
    nd_div.delta_y = node->divider.delta_y;

    side = PointOnDividingLineSide(view_x, view_y, &nd_div);

    // Recursively divide front space.
    if (RendererCheckBBox(node->bounding_boxes[side]))
        RendererWalkBspNode(node->children[side]);

    // Recursively divide back space.
    if (RendererCheckBBox(node->bounding_boxes[side ^ 1]))
        RendererWalkBspNode(node->children[side ^ 1]);
}

//
// RenderTrueBsp
//
// OpenGL BSP rendering.  Initialises all structures, then walks the
// BSP tree collecting information, then renders each subsector:
// firstly front to back (drawing all solid walls & planes) and then
// from back to front (drawing everything else, sprites etc..).
//
static void RenderTrueBsp(void)
{
    EDGE_ZoneScoped;

    FuzzUpdate();

    ClearBSP();
    OcclusionClear();

    draw_subsector_list.clear();

    Player *v_player = view_camera_map_object->player_;

    SetupMatrices3d();

    frame_texture_ids.clear();
    frame_texture_ids.reserve(1024);

    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // needed for drawing the sky
    BeginSky();

    // walk the bsp tree
    RendererWalkBspNode(root_node);

    FinishSky();

    RenderState *state = GetRenderState();
    state->SetDefaultStateFull();

    RenderSubList(draw_subsector_list);

    state->SetDefaultStateFull();

    glDisable(GL_DEPTH_TEST);

    // now draw 2D stuff like psprites, and add effects
    SetupWorldMatrices2D();

    if (v_player)
    {
        RenderWeaponSprites(v_player);
        SetupMatrices2D();
        RenderCrosshair(v_player);
    }

#if (DEBUG >= 3)
    LogDebug("\n\n");
#endif
}

static void InitializeCamera(MapObject *mo, bool full_height, float expand_w)
{
    float fov = glm_clamp(5, field_of_view.f_, 175);

    wave_now    = level_time_elapsed / 100.0f;
    plane_z_bob = sine_table[(int)((kWavetableIncrement + wave_now) * kSineTableSize) & (kSineTableMask)];

    view_x_slope = tanf(90.0f * GLM_PIf / 360.0f);

    if (full_height)
        view_y_slope = kDoomYSlopeFull;
    else
        view_y_slope = kDoomYSlope;

    if (!AlmostEquals(fov, 90.0f))
    {
        float new_slope = tanf(fov * GLM_PIf / 360.0f);

        view_y_slope *= new_slope / view_x_slope;
        view_x_slope = new_slope;
    }

    view_is_zoomed = false;

    if (mo->player_ && mo->player_->zoom_field_of_view_ > 0)
    {
        view_is_zoomed = true;

        float new_slope = tanf(mo->player_->zoom_field_of_view_ * GLM_PIf / 360.0f);

        view_y_slope *= new_slope / view_x_slope;
        view_x_slope = new_slope;
    }

    // wide-screen adjustment
    widescreen_view_width_multiplier = expand_w;

    view_x_slope *= widescreen_view_width_multiplier;

    view_x     = mo->x;
    view_y     = mo->y;
    view_z     = mo->z;
    view_angle = mo->angle_;

    if (mo->player_)
        view_z += mo->player_->view_z_;
    else
        view_z += mo->height_ * 9 / 10;

    view_subsector      = mo->subsector_;
    view_vertical_angle = mo->vertical_angle_;
    view_properties     = GetViewPointProperties(view_subsector, view_z);

    if (mo->player_)
    {
        view_vertical_angle += epi::BAMFromATan(mo->player_->kick_offset_);

        // No heads above the ceiling
        if (view_z > mo->player_->map_object_->ceiling_z_ - 2)
            view_z = mo->player_->map_object_->ceiling_z_ - 2;

        // No heads below the floor, please
        if (view_z < mo->player_->map_object_->floor_z_ + 2)
            view_z = mo->player_->map_object_->floor_z_ + 2;
    }

    // do some more stuff
    view_sine   = epi::BAMSin(view_angle);
    view_cosine = epi::BAMCos(view_angle);

    float lk_sin = epi::BAMSin(view_vertical_angle);
    float lk_cos = epi::BAMCos(view_vertical_angle);

    view_forward.x = lk_cos * view_cosine;
    view_forward.y = lk_cos * view_sine;
    view_forward.z = lk_sin;

    view_up.x = -lk_sin * view_cosine;
    view_up.y = -lk_sin * view_sine;
    view_up.z = lk_cos;

    // cross product
    view_right.x = view_forward.y * view_up.z - view_up.y * view_forward.z;
    view_right.y = view_forward.z * view_up.x - view_up.z * view_forward.x;
    view_right.z = view_forward.x * view_up.y - view_up.x * view_forward.y;

    // compute the 1D projection of the view angle
    BAMAngle oned_side_angle;
    {
        float k, d;

        // k is just the mlook angle (in radians)
        k = epi::DegreesFromBAM(view_vertical_angle);
        if (k > 180.0)
            k -= 360.0;
        k = k * GLM_PIf / 180.0f;

        sprite_skew = tanf((-k) / 2.0);

        k = fabs(k);

        // d is just the distance horizontally forward from the eye to
        // the top/bottom edge of the view rectangle.
        d = cos(k) - sin(k) * view_y_slope;

        oned_side_angle = (d <= 0.01f) ? kBAMAngle180 : epi::BAMFromATan(view_x_slope / d);
    }

    // setup clip angles
    if (oned_side_angle != kBAMAngle180)
    {
        clip_left  = 0 + oned_side_angle;
        clip_right = 0 - oned_side_angle;
        clip_scope = clip_left - clip_right;
    }
    else
    {
        // not clipping to the viewport.  Dummy values.
        clip_scope = kBAMAngle180;
        clip_left  = 0 + kBAMAngle45;
        clip_right = uint32_t(0 - kBAMAngle45);
    }
}

void RenderView(int x, int y, int w, int h, MapObject *camera, bool full_height, float expand_w)
{
    EDGE_ZoneScoped;

    view_window_x      = x;
    view_window_y      = y;
    view_window_width  = w;
    view_window_height = h;

    view_camera_map_object = camera;

    // Load the details for the camera
    InitializeCamera(camera, full_height, expand_w);

    // Profiling
    render_frame_count++;
    valid_count++;

    RenderTrueBsp();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
