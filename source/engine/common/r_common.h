

#pragma once

#include "m_fixed.h"
#include "tables.h"

// killough 10/98: special mask indicates sky flat comes from sidedef
#define PL_SKYFLAT (0x80000000)

// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS 32

#define FX_ROCKET          0x00000001
#define FX_GRENADE         0x00000002
#define FX_VISIBILITYPULSE 0x00000040

#define FX_FOUNTAINMASK   0x00070000
#define FX_FOUNTAINSHIFT  16
#define FX_REDFOUNTAIN    0x00010000
#define FX_GREENFOUNTAIN  0x00020000
#define FX_BLUEFOUNTAIN   0x00030000
#define FX_YELLOWFOUNTAIN 0x00040000
#define FX_PURPLEFOUNTAIN 0x00050000
#define FX_BLACKFOUNTAIN  0x00060000
#define FX_WHITEFOUNTAIN  0x00070000

extern int32_t validcount;
extern fixed_t render_lerp_amount;

void R_RotatePoint(fixed_t x, fixed_t y, angle_t ang, fixed_t &tx, fixed_t &ty);

// 2/1/10: Updated (from EE) to restore vanilla style, with tweak for overflow tolerance
angle_t R_PointToAngle2(fixed_t viewx, fixed_t viewy, fixed_t x, fixed_t y);

bool R_AlignFlat(int32_t linenum, int32_t side, int32_t fc);

// Called by exit code.
void STACK_ARGS R_Shutdown();

void R_ExitLevel();

// Utility function,
//	called by R_PointToAngle.
uint32_t inline SlopeDiv(uint32_t num, uint32_t den)
{
    uint32_t ans;

    if (den < 512)
        return SLOPERANGE;

    ans = (num << 3) / (den >> 8);

    return ans <= SLOPERANGE ? ans : SLOPERANGE;
}
