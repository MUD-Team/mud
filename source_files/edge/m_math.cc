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
// PointInTriangle is adapted from the PNPOLY algorithm with the following
// license:
//
// Copyright (c) 1970-2003, Wm. Randolph Franklin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimers. Redistributions in binary
// form must reproduce the above copyright notice in the documentation and/or
// other materials provided with the distribution. The name of W. Randolph
// Franklin may not be used to endorse or promote products derived from this
// Software without specific prior written permission. THE SOFTWARE IS PROVIDED
// "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "m_math.h"

#include <math.h>

#include <vector>
#ifdef __APPLE__
#include <math.h>
#endif

void BAMAngleToMatrix(BAMAngle ang, vec2s *x, vec2s *y)
{
    x->x = epi::BAMCos(ang);
    x->y = epi::BAMSin(ang);

    y->x = -x->y;
    y->y = x->x;
}

vec3s TripleCrossProduct(vec3s v1, vec3s v2, vec3s v3)
{
    return glms_vec3_cross(glms_vec3_sub(v2, v1), glms_vec3_sub(v3, v1));
}

// If the plane normal is precalculated; otherwise use the other version
vec3s LinePlaneIntersection(vec3s line_a, vec3s line_b, vec3s plane_c, vec3s plane_normal)
{
    float    n             = glms_vec3_dot(plane_normal, glms_vec3_sub(plane_c, line_a));
    vec3s line_subtract = glms_vec3_sub(line_b, line_a);
    float    d             = glms_vec3_dot(plane_normal, line_subtract);
    return glms_vec3_add(line_a, glms_vec3_scale(line_subtract, n / d));
}

vec3s LinePlaneIntersection(vec3s line_a, vec3s line_b, vec3s plane_a, vec3s plane_b,
                                   vec3s plane_c)
{
    vec3s plane_normal  = TripleCrossProduct(plane_a, plane_b, plane_c);
    float    n             = glms_vec3_dot(plane_normal, glms_vec3_sub(plane_c, line_a));
    vec3s line_subtract = glms_vec3_sub(line_b, line_a);
    float    d             = glms_vec3_dot(plane_normal, line_subtract);
    return glms_vec3_add(line_a, glms_vec3_scale(line_subtract, n / d));
}

int PointInTriangle(vec2s v1, vec2s v2, vec2s v3, vec2s test)
{
    std::vector<vec2s> tri_vec = {v1, v2, v3};
    int                   i       = 0;
    int                   j       = 0;
    int                   c       = 0;
    for (i = 0, j = 2; i < 3; j = i++)
    {
        if (((tri_vec[i].y > test.y) != (tri_vec[j].y > test.y)) &&
            (test.x <
             (tri_vec[j].x - tri_vec[i].x) * (test.y - tri_vec[i].y) / (tri_vec[j].y - tri_vec[i].y) + tri_vec[i].x))
            c = !c;
    }
    return c;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
