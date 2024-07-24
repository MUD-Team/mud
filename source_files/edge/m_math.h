//----------------------------------------------------------------------------
//  EDGE Floating Point Math Stuff
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

#pragma once

#include "cglm/struct.h"
#include "epi_bam.h"

vec3s TripleCrossProduct(const vec3s &v1, const vec3s &v2, const vec3s &v3);
vec3s LinePlaneIntersection(const vec3s &line_a, const vec3s &line_b, const vec3s &plane_c, const vec3s &plane_normal);
vec3s LinePlaneIntersection(const vec3s &line_a, const vec3s &line_b, const vec3s &plane_a, const vec3s &plane_b,
                                   const vec3s &plane_c);
void     BAMAngleToMatrix(BAMAngle ang, vec2s *x, vec2s *y);
int      PointInTriangle(const vec2s &v1, const vec2s &v2, const vec2s &v3, const vec2s &test);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
