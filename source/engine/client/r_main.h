// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 1a6aa1bbd336eae8f13f52ef434740954ae97bdd $
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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------

#pragma once

#include "d_player.h"
#include "doomtype.h"
#include "g_level.h"
#include "m_vectors.h"
#include "r_common.h"
#include "v_palette.h"
#include "v_video.h"

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//	and other lighting effects (sector ambient, flash).
//

// Lighting constants.
// Now why not 32 levels here?
#define LIGHTLEVELS   16
#define LIGHTSEGSHIFT 4

#define MAXLIGHTSCALE     48
#define LIGHTSCALEMULBITS 8 // [RH] for hires lighting fix
#define LIGHTSCALESHIFT   (12 + LIGHTSCALEMULBITS)
#define MAXLIGHTZ         128
#define LIGHTZSHIFT       20

extern int32_t negonearray[MAXWIDTH];
extern int32_t viewheightarray[MAXWIDTH];

//
// POV related.
//
extern fixed_t viewcos;
extern fixed_t viewsin;

extern int32_t viewwidth;
extern int32_t viewheight;
extern int32_t viewwindowx;
extern int32_t viewwindowy;

extern bool r_fakingunderwater;
extern bool r_underwater;

extern int32_t centerx;
extern int32_t centery;

extern fixed_t centerxfrac;
extern fixed_t centeryfrac;
extern fixed_t yaspectmul;

extern shaderef_t basecolormap; // [RH] Colormap for sector currently being drawn

extern int32_t linecount;
extern int32_t loopcount;

// [SL] Current color blending values (including palette effects)
extern fargb_t blend_color;

void   R_SetSectorBlend(const argb_t color);
void   R_ClearSectorBlend();
argb_t R_GetSectorBlend();

// [RH] Changed from shaderef_t* to int32_t.
extern int32_t scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
extern int32_t scalelightfixed[MAXLIGHTSCALE];
extern int32_t zlight[LIGHTLEVELS][MAXLIGHTZ];

extern int32_t    extralight;
extern bool       foggy;
extern int32_t    fixedlightlev;
extern shaderef_t fixedcolormap;

extern int32_t lightscalexmul; // [RH] for hires lighting fix
extern int32_t lightscaleymul;
//
// Function pointers to switch refresh/drawing functions.
//
extern void (*colfunc)(void);
extern void (*spanfunc)(void);
extern void (*spanslopefunc)(void);

//
// Utility functions.

int32_t R_PointOnSide(fixed_t x, fixed_t y, const node_t *node);
int32_t R_PointOnSide(fixed_t x, fixed_t y, fixed_t xl, fixed_t yl, fixed_t xh, fixed_t yh);
int32_t R_PointOnSegSide(fixed_t x, fixed_t y, const seg_t *line);
bool    R_PointOnLine(fixed_t x, fixed_t y, fixed_t xl, fixed_t yl, fixed_t xh, fixed_t yh);

angle_t R_PointToAngle(fixed_t x, fixed_t y);

fixed_t R_PointToDist(fixed_t x, fixed_t y);

int32_t R_ProjectPointX(fixed_t x, fixed_t y);
int32_t R_ProjectPointY(fixed_t z, fixed_t y);
bool    R_CheckProjectionX(int32_t &x1, int32_t &x2);
bool    R_CheckProjectionY(int32_t &y1, int32_t &y2);

bool R_ClipLineToFrustum(const v2fixed_t *v1, const v2fixed_t *v2, fixed_t clipdist, int32_t &lclip, int32_t &rclip);

void R_ClipLine(const v2fixed_t *in1, const v2fixed_t *in2, int32_t lclip, int32_t rclip, v2fixed_t *out1,
                v2fixed_t *out2);
void R_ClipLine(const vertex_t *in1, const vertex_t *in2, int32_t lclip, int32_t rclip, v2fixed_t *out1,
                v2fixed_t *out2);

subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);

void R_AddPointToBox(int32_t x, int32_t y, fixed_t *box);

fixed_t R_PointToDist2(fixed_t dx, fixed_t dy);
void    R_SetFOV(float fov, bool force);
float   R_GetFOV(void);

#define WIDE_STRETCH 0
#define WIDE_ZOOM    1
#define WIDE_TRUE    2

int32_t R_GetWidescreen(void);

//
// REFRESH - the actual rendering functions.
//

// Called by G_Drawer.
void R_RenderPlayerView(player_t *player);

// Called by M_Responder.
void R_SetViewSize(int32_t blocks);

class IRenderSurface;
IRenderSurface *R_GetRenderingSurface();

bool R_BorderVisible();
bool R_StatusBarVisible();
bool R_DemoBarInvisible();

int32_t R_ViewWidth(int32_t width, int32_t height);
int32_t R_ViewHeight(int32_t width, int32_t height);
int32_t R_ViewWindowX(int32_t width, int32_t height);
int32_t R_ViewWindowY(int32_t width, int32_t height);

void R_ForceViewWindowResize();

void R_ResetDrawFuncs();
void R_SetFuzzDrawFuncs();
void R_SetLucentDrawFuncs();

inline uint8_t shaderef_t::ramp() const
{
    if (m_mapnum >= NUMCOLORMAPS)
        return 0;

    int32_t index = clamp(m_mapnum * 256 / NUMCOLORMAPS, 0, 255);
    return m_colors->ramp[index];
}

void R_DrawLine(const v3fixed_t *inpt1, const v3fixed_t *inpt2, uint8_t color);
