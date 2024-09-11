//----------------------------------------------------------------------------
//  EDGE Generalised Image Handling
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
// -AJA- 2000/06/25: Began this image generalisation, based on Erik
//       Sandberg's w_textur.c/h code.
//
// TODO HERE:
//   -  faster search methods.
//   -  do some optimisation
//

#include <limits.h>

#include "dm_state.h"
#include "e_main.h"
#include "e_search.h"
#include "epi.h"
#include "epi_endian.h"
#include "epi_filesystem.h"
#include "i_defs_gl.h"
#include "im_data.h"
#include "im_funcs.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_colormap.h"
#include "r_defs.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_sky.h"
#include "r_texgl.h"
#include "w_files.h"


// Dummy image, for when texture/flat/graphic is unknown.  Row major
// order.  Could be packed, but why bother ?
static constexpr uint8_t dummy_graphic[kDummyImageSize * kDummyImageSize] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


//------------------------------------------------------------------------

//
//  BLOCK READING STUFF
//

//
// ReadPatchAsBlock
//
// Loads a patch from the wad and returns the image block for it.
// Very similiar to ReadTextureAsBlock() above.  Doesn't do any
// mipmapping (this is too "raw" if you follow).
//
//---- This routine will also update the `solid' flag
//---- if it turns out to be 100% solid.
//
static ImageData *ReadPatchAsEpiBlock(Image *rim)
{
    EPI_ASSERT(rim->source_type_ == kImageSourceGraphic || rim->source_type_ == kImageSourceSprite);

    const char *packfile_name = rim->source_.graphic.packfile_name;

    epi::File *f = nullptr;

    if (packfile_name)
        f = OpenPackFile(packfile_name, "");
    else
        FatalError("No pack file name given for image: %s\n", rim->name_.c_str());

    ImageData *img = LoadImageData(f);

    // close it
    delete f;

    if (!img)
        FatalError("Error loading image from file: %s\n", packfile_name);

    return img;
}

//
// ReadDummyAsBlock
//
// Creates a dummy image.
//
static ImageData *ReadDummyAsEpiBlock(Image *rim)
{
    EPI_ASSERT(rim->source_type_ == kImageSourceDummy);
    EPI_ASSERT(rim->actual_width_ == rim->total_width_);
    EPI_ASSERT(rim->actual_height_ == rim->total_height_);
    EPI_ASSERT(rim->total_width_ == kDummyImageSize);
    EPI_ASSERT(rim->total_height_ == kDummyImageSize);

    ImageData *img = new ImageData(kDummyImageSize, kDummyImageSize, 4);

    // copy pixels
    for (int y = 0; y < kDummyImageSize; y++)
        for (int x = 0; x < kDummyImageSize; x++)
        {
            uint8_t *dest_pix = img->PixelAt(x, y);

            if (dummy_graphic[(kDummyImageSize - 1 - y) * kDummyImageSize + x])
            {
                *dest_pix++ = (rim->source_.dummy.fg & 0xFF0000) >> 16;
                *dest_pix++ = (rim->source_.dummy.fg & 0x00FF00) >> 8;
                *dest_pix++ = (rim->source_.dummy.fg & 0x0000FF);
                *dest_pix++ = 255;
            }
            else if (rim->source_.dummy.bg == kTransparentPixelIndex)
            {
                *dest_pix++ = 0;
                *dest_pix++ = 0;
                *dest_pix++ = 0;
                *dest_pix++ = 0;
            }
            else
            {
                *dest_pix++ = (rim->source_.dummy.bg & 0xFF0000) >> 16;
                *dest_pix++ = (rim->source_.dummy.bg & 0x00FF00) >> 8;
                *dest_pix++ = (rim->source_.dummy.bg & 0x0000FF);
                *dest_pix++ = 255;
            }
        }

    return img;
}

static ImageData *CreateUserColourImage(Image *rim, ImageDefinition *def)
{
    int tw = GLM_MAX(rim->total_width_, 1);
    int th = GLM_MAX(rim->total_height_, 1);

    ImageData *img = new ImageData(tw, th, 3);

    uint8_t *dest = img->pixels_;

    for (int y = 0; y < img->height_; y++)
        for (int x = 0; x < img->width_; x++)
        {
            *dest++ = epi::GetRGBARed(def->colour_);
            *dest++ = epi::GetRGBAGreen(def->colour_);
            *dest++ = epi::GetRGBABlue(def->colour_);
        }

    return img;
}

epi::File *OpenUserFileOrLump(ImageDefinition *def)
{
    switch (def->type_)
    {
    case kImageDataPackage:
        return OpenPackFile(def->info_, "");

    default:
        return nullptr;
    }
}

static ImageData *CreateUserFileImage(Image *rim, ImageDefinition *def)
{
    epi::File *f = OpenUserFileOrLump(def);

    if (!f)
        FatalError("Missing image file: %s\n", def->info_.c_str());

    ImageData *img = LoadImageData(f);

    // close it
    delete f;

    if (!img)
        FatalError("Error occurred loading image file: %s\n", def->info_.c_str());

    rim->opacity_ = DetermineOpacity(img);

    if (def->fix_trans_ == kTransparencyFixBlacken)
        BlackenClearAreas(img);

    EPI_ASSERT(rim->total_width_ == img->width_);
    EPI_ASSERT(rim->total_height_ == img->height_);

    // CW: Textures MUST tile! If actual size not total size, manually tile
    // [ AJA: this does not make them tile, just fills in the black gaps ]
    if (rim->opacity_ == kOpacitySolid)
    {
        img->FillMarginX(rim->actual_width_);
        img->FillMarginY(rim->actual_height_);
    }

    return img;
}

//
// ReadUserAsEpiBlock
//
// Loads or Creates the user defined image.
// Doesn't do any mipmapping (this is too "raw" if you follow).
//
static ImageData *ReadUserAsEpiBlock(Image *rim)
{
    EPI_ASSERT(rim->source_type_ == kImageSourceUser);

    ImageDefinition *def = rim->source_.user.def;

    switch (def->type_)
    {
    case kImageDataColor:
        return CreateUserColourImage(rim, def);

    case kImageDataPackage:
        return CreateUserFileImage(rim, def);

    default:
        FatalError("ReadUserAsEpiBlock: Coding error, unknown type %d\n", def->type_);
    }

    return nullptr; /* NOT REACHED */
}

//
// ReadAsEpiBlock
//
// Read the image from the wad into an image_data_c class.
// Mainly just a switch to more specialised image readers.
//
// Never returns nullptr.
//
ImageData *ReadAsEpiBlock(Image *rim)
{
    switch (rim->source_type_)
    {
    case kImageSourceGraphic:
    case kImageSourceSprite:
        return ReadPatchAsEpiBlock(rim);

    case kImageSourceDummy:
        return ReadDummyAsEpiBlock(rim);

    case kImageSourceUser:
        return ReadUserAsEpiBlock(rim);

    default:
        FatalError("ReadAsBlock: unknown source_type %d !\n", rim->source_type_);
        return nullptr;
    }
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
