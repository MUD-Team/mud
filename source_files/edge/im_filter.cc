//------------------------------------------------------------------------
//  EDGE Image Filtering/Scaling
//------------------------------------------------------------------------
//
//  Copyright (c) 2007-2024 The EDGE Team.
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
//  Blur is based on "C++ implementation of a fast Gaussian blur algorithm by
//    Ivan Kutskir - Integer Version"
//
//  Copyright (C) 2017 Basile Fraboni
//  Copyright (C) 2014 Ivan Kutskir
//  All Rights Reserved
//  You may use, distribute and modify this code under the
//  terms of the MIT license. For further details please refer
//  to : https://mit-license.org/
//
//----------------------------------------------------------------------------

#include "im_filter.h"

#include <math.h>
#include <string.h>

#include <algorithm>

#include "HandmadeMath.h"
#include "epi.h"

static void SigmaToBox(int boxes[], float sigma, int n)
{
    // ideal filter width
    float wi = sqrt((12 * sigma * sigma / n) + 1);
    int   wl = floor(wi);
    if (wl % 2 == 0)
        wl--;
    int wu = wl + 2;

    float mi = (12 * sigma * sigma - n * wl * wl - 4 * n * wl - 3 * n) / (-4 * wl - 4);
    int   m  = round(mi);

    for (int i = 0; i < n; i++)
        boxes[i] = ((i < m ? wl : wu) - 1) / 2;
}

static void HorizontalBlurRGB(uint8_t *in, uint8_t *out, int w, int h, int c, int r)
{
    float iarr = 1.f / (r + r + 1);
    for (int i = 0; i < h; i++)
    {
        int ti = i * w;
        int li = ti;
        int ri = ti + r;

        int fv[3]  = {in[ti * c + 0], in[ti * c + 1], in[ti * c + 2]};
        int lv[3]  = {in[(ti + w - 1) * c + 0], in[(ti + w - 1) * c + 1], in[(ti + w - 1) * c + 2]};
        int val[3] = {(r + 1) * fv[0], (r + 1) * fv[1], (r + 1) * fv[2]};

        for (int j = 0; j < r; j++)
        {
            val[0] += in[(ti + j) * c + 0];
            val[1] += in[(ti + j) * c + 1];
            val[2] += in[(ti + j) * c + 2];
        }

        for (int j = 0; j <= r; j++, ri++, ti++)
        {
            val[0] += in[ri * c + 0] - fv[0];
            val[1] += in[ri * c + 1] - fv[1];
            val[2] += in[ri * c + 2] - fv[2];
            out[ti * c + 0] = round(val[0] * iarr);
            out[ti * c + 1] = round(val[1] * iarr);
            out[ti * c + 2] = round(val[2] * iarr);
        }

        for (int j = r + 1; j < w - r; j++, ri++, ti++, li++)
        {
            val[0] += in[ri * c + 0] - in[li * c + 0];
            val[1] += in[ri * c + 1] - in[li * c + 1];
            val[2] += in[ri * c + 2] - in[li * c + 2];
            out[ti * c + 0] = round(val[0] * iarr);
            out[ti * c + 1] = round(val[1] * iarr);
            out[ti * c + 2] = round(val[2] * iarr);
        }

        for (int j = w - r; j < w; j++, ti++, li++)
        {
            val[0] += lv[0] - in[li * c + 0];
            val[1] += lv[1] - in[li * c + 1];
            val[2] += lv[2] - in[li * c + 2];
            out[ti * c + 0] = round(val[0] * iarr);
            out[ti * c + 1] = round(val[1] * iarr);
            out[ti * c + 2] = round(val[2] * iarr);
        }
    }
}

static void TotalBlurRGB(uint8_t *in, uint8_t *out, int w, int h, int c, int r)
{
    // radius range on either side of a pixel + the pixel itself
    float iarr = 1.f / (r + r + 1);
    for (int i = 0; i < w; i++)
    {
        int ti = i;
        int li = ti;
        int ri = ti + r * w;

        int fv[3]  = {in[ti * c + 0], in[ti * c + 1], in[ti * c + 2]};
        int lv[3]  = {in[(ti + w * (h - 1)) * c + 0], in[(ti + w * (h - 1)) * c + 1], in[(ti + w * (h - 1)) * c + 2]};
        int val[3] = {(r + 1) * fv[0], (r + 1) * fv[1], (r + 1) * fv[2]};

        for (int j = 0; j < r; j++)
        {
            val[0] += in[(ti + j * w) * c + 0];
            val[1] += in[(ti + j * w) * c + 1];
            val[2] += in[(ti + j * w) * c + 2];
        }

        for (int j = 0; j <= r; j++, ri += w, ti += w)
        {
            val[0] += in[ri * c + 0] - fv[0];
            val[1] += in[ri * c + 1] - fv[1];
            val[2] += in[ri * c + 2] - fv[2];
            out[ti * c + 0] = round(val[0] * iarr);
            out[ti * c + 1] = round(val[1] * iarr);
            out[ti * c + 2] = round(val[2] * iarr);
        }

        for (int j = r + 1; j < h - r; j++, ri += w, ti += w, li += w)
        {
            val[0] += in[ri * c + 0] - in[li * c + 0];
            val[1] += in[ri * c + 1] - in[li * c + 1];
            val[2] += in[ri * c + 2] - in[li * c + 2];
            out[ti * c + 0] = round(val[0] * iarr);
            out[ti * c + 1] = round(val[1] * iarr);
            out[ti * c + 2] = round(val[2] * iarr);
        }

        for (int j = h - r; j < h; j++, ti += w, li += w)
        {
            val[0] += lv[0] - in[li * c + 0];
            val[1] += lv[1] - in[li * c + 1];
            val[2] += lv[2] - in[li * c + 2];
            out[ti * c + 0] = round(val[0] * iarr);
            out[ti * c + 1] = round(val[1] * iarr);
            out[ti * c + 2] = round(val[2] * iarr);
        }
    }
}

static void BoxBlurRGB(uint8_t *&in, uint8_t *&out, int w, int h, int c, int r)
{
    std::swap(in, out);
    HorizontalBlurRGB(out, in, w, h, c, r);
    TotalBlurRGB(in, out, w, h, c, r);
}

ImageData *ImageBlur(ImageData *image, float sigma)
{
    EPI_ASSERT(image->depth_ >= 3);

    int w = image->width_;
    int h = image->height_;
    int c = image->depth_;

    ImageData *result = new ImageData(w, h, c);

    int box;
    SigmaToBox(&box, sigma, 1);
    BoxBlurRGB(image->pixels_, result->pixels_, w, h, c, box);

    return result;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
