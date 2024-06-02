
#include "r_common.h"

// increment every time a check is made
int32_t validcount = 1;

//
// precalculated math tables
//

const fixed_t *finecosine = &finesine[FINEANGLES / 4];

fixed_t render_lerp_amount;

#define R_P2ATHRESHOLD (INT_MAX / 4)

//
// R_PointToAngle
//
// To get a global angle from cartesian coordinates, the coordinates are
// flipped until they are in the first octant of the coordinate system,
// then the y (<=x) is scaled and divided by x to get a tangent (slope)
// value which is looked up in the tantoangle[] table.
//
// This version is from prboom-plus
//
// clang-format off
angle_t R_PointToAngle2(fixed_t viewx, fixed_t viewy, fixed_t x, fixed_t y)
{
	return (y -= viewy, (x -= viewx) || y) ?
		x >= 0 ?
			y >= 0 ?
				(x > y) ? tantoangle[SlopeDiv(y,x)] :						// octant 0
					ANG90-1-tantoangle[SlopeDiv(x,y)] :						// octant 1
				x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :				// octant 8
						ANG270+tantoangle[SlopeDiv(x,y)] :					// octant 7
			y >= 0 ? (x = -x) > y ? ANG180-1-tantoangle[SlopeDiv(y,x)] :	// octant 3
							ANG90 + tantoangle[SlopeDiv(x,y)] :				// octant 2
				(x = -x) > (y = -y) ? ANG180+tantoangle[ SlopeDiv(y,x)] :	// octant 4
								ANG270-1-tantoangle[SlopeDiv(x,y)] :		// octant 5
		0;
}
// clang-format on

void R_RotatePoint(fixed_t x, fixed_t y, angle_t ang, fixed_t &tx, fixed_t &ty)
{
    int32_t index = ang >> ANGLETOFINESHIFT;

    tx = FixedMul(x, finecosine[index]) - FixedMul(y, finesine[index]);
    ty = FixedMul(x, finesine[index]) + FixedMul(y, finecosine[index]);
}
