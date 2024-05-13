// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 03a59f1f59c20080abc86a29a1ca814324acdc28 $
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
//	Rendering main loop and setup functions,
//	 utility functions (BSP, geometry, trigonometry).
//	See tables.c, too.
//
//-----------------------------------------------------------------------------

#include <math.h>

#include "m_bbox.h"
#include "m_random.h"
#include "mud_includes.h"
#include "r_local.h"
#include "v_video.h"

#define DISTMAP 2

void R_SpanInitData();

extern int32_t          *walllights;
extern dyncolormap_t NormalLight;

// [Russell] - Server expects these to exist
// [Russell] - Doesn't get used serverside
uint8_t *translationtables;
uint8_t  bosstable[256];

fixed_t FocalLengthX;
fixed_t FocalLengthY;
int32_t     viewangleoffset;

// increment every time a check is made
int32_t validcount = 1;

int32_t centerx;
int32_t centery;

fixed_t centerxfrac;
fixed_t centeryfrac;
fixed_t yaspectmul;

// just for profiling purposes
int32_t framecount;
int32_t linecount;
int32_t loopcount;

fixed_t viewx;
fixed_t viewy;
fixed_t viewz;

angle_t viewangle;

fixed_t viewcos;
fixed_t viewsin;

AActor *camera; // [RH] camera to draw from. doesn't have to be a player

//
// precalculated math tables
//

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int32_t viewangletox[FINEANGLES / 2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t *xtoviewangle;

const fixed_t *finecosine = &finesine[FINEANGLES / 4];

int32_t scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
int32_t scalelightfixed[MAXLIGHTSCALE];
int32_t zlight[LIGHTLEVELS][MAXLIGHTZ];

int32_t lightscalexmul; // [RH] used to keep hires modes dark enough
int32_t lightscaleymul;

// bumped light from gun blasts
int32_t extralight;

// [RH] ignore extralight and fullbright
bool foggy;

fixed_t freelookviewheight;

fixed_t render_lerp_amount;

uint32_t R_OldBlend = ~0;

void (*colfunc)(void);
void (*basecolfunc)(void);
void (*fuzzcolfunc)(void);
void (*lucentcolfunc)(void);
void (*transcolfunc)(void);
void (*tlatedlucentcolfunc)(void);
void (*spanfunc)(void);

void (*hcolfunc_pre)(void);
void (*hcolfunc_post1)(int32_t hx, int32_t sx, int32_t yl, int32_t yh);
void (*hcolfunc_post2)(int32_t hx, int32_t sx, int32_t yl, int32_t yh);
void (*hcolfunc_post4)(int32_t sx, int32_t yl, int32_t yh);

#define R_P2ATHRESHOLD (INT_MAX / 4)

//
// R_PointToAngle
//
// To get a global angle from cartesian coordinates, the coordinates are
// flipped until they are in the first octant of the coordinate system,
// then the y (<=x) is scaled and divided by x to get a tangent (slope)
// value which is looked up in the tantoangle[] table.
//

angle_t R_PointToAngle2(fixed_t viewx, fixed_t viewy, fixed_t x, fixed_t y)
{
    x -= viewx;
    y -= viewy;

    if ((x | y) == 0)
        return 0;

    if (x < R_P2ATHRESHOLD && x > -R_P2ATHRESHOLD && y < R_P2ATHRESHOLD && y > -R_P2ATHRESHOLD)
    {
        if (x >= 0)
        {
            if (y >= 0)
            {
                if (x > y)
                {
                    // octant 0
                    return tantoangle_acc[SlopeDiv(y, x)];
                }
                else
                {
                    // octant 1
                    return ANG90 - 1 - tantoangle_acc[SlopeDiv(x, y)];
                }
            }
            else // y < 0
            {
                y = -y;

                if (x > y)
                {
                    // octant 8
                    return 0 - tantoangle_acc[SlopeDiv(y, x)];
                }
                else
                {
                    // octant 7
                    return ANG270 + tantoangle_acc[SlopeDiv(x, y)];
                }
            }
        }
        else // x < 0
        {
            x = -x;

            if (y >= 0)
            {
                if (x > y)
                {
                    // octant 3
                    return ANG180 - 1 - tantoangle_acc[SlopeDiv(y, x)];
                }
                else
                {
                    // octant 2
                    return ANG90 + tantoangle_acc[SlopeDiv(x, y)];
                }
            }
            else // y < 0
            {
                y = -y;

                if (x > y)
                {
                    // octant 4
                    return ANG180 + tantoangle_acc[SlopeDiv(y, x)];
                }
                else
                {
                    // octant 5
                    return ANG270 - 1 - tantoangle_acc[SlopeDiv(x, y)];
                }
            }
        }
    }
    else
    {
        return (angle_t)(atan2((double)y, (double)x) * (ANG180 / PI));
    }

    return 0;
}

//
// R_PointToAngle - wrapper around R_PointToAngle2
//
angle_t R_PointToAngle(fixed_t x, fixed_t y)
{
    return R_PointToAngle2(viewx, viewy, x, y);
}

void R_RotatePoint(fixed_t x, fixed_t y, angle_t ang, fixed_t &tx, fixed_t &ty)
{
    int32_t index = ang >> ANGLETOFINESHIFT;

    tx = FixedMul(x, finecosine[index]) - FixedMul(y, finesine[index]);
    ty = FixedMul(x, finesine[index]) + FixedMul(y, finecosine[index]);
}

//
//
// R_Init
//
//

void R_Init(void)
{
    R_InitData();

    framecount = 0;
}

VERSION_CONTROL(r_main_cpp, "$Id: 03a59f1f59c20080abc86a29a1ca814324acdc28 $")
