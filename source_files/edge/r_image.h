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

#pragma once

#include <list>
#include <string>
#include <vector>

#include "ddf_image.h"
#include "ddf_main.h"
#include "r_defs.h"
#include "r_state.h"
#include "w_epk.h"

struct TextureDefinition;

// the transparent pixel value we use
constexpr uint8_t kTransparentPixelIndex = 247;

// size of dummy replacements
constexpr uint8_t kDummyImageSize = 16;

enum ImageOpacity
{
    kOpacityUnknown = 0,
    kOpacitySolid   = 1, // utterly solid (alpha = 255 everywhere)
    kOpacityMasked  = 2, // only uses alpha 255 and 0
    kOpacityComplex = 3, // uses full range of alpha values
};

class Image
{
  public:
    // actual image size.  Images that are smaller than their total size
    // are located in the bottom left corner, cannot tile, and are padded
    // with black pixels if solid, or transparent pixels otherwise.
    uint16_t actual_width_;
    uint16_t actual_height_;

    // total image size, must be a power of two on each axis.
    uint16_t total_width_;
    uint16_t total_height_;

    // ratio of actual w/h to total w/h of the image for calculating texcoords
    float width_ratio_;
    float height_ratio_;

    // offset values.  Only used for sprites and on-screen patches.
    float offset_x_;
    float offset_y_;

    // scale values, where 1.0f is normal.  Higher values stretch the
    // image (on the wall/floor), lower values shrink it.
    float scale_x_;
    float scale_y_;

    // one of the kOpacityXXX values
    int opacity_;

    bool is_font_;

    // For fully transparent images
    bool is_empty_;

    bool grayscale_ = false;

    int hsv_rotation_   = 0;
    int hsv_saturation_ = -1;
    int hsv_value_      = 0;

    // --- information about where this image came from ---
    std::string name_;

    int source_type_; // image_source_e

    union {
        // case kImageSourceGraphic:
        // case kImageSourceSprite:
        struct
        {
            char        *packfile_name;
            ImageSpecial special;
            ImageNamespace belong;
        } graphic;

        // case kImageSourceDummy:
        struct
        {
            RGBAColor fg;
            RGBAColor bg;
        } dummy;

        // case kImageSourceUser:
        struct
        {
            ImageDefinition *def;
        } user;
    } source_;

    // --- information about caching ---

    std::vector<struct CachedImage *> cache_;

    // --- animation info ---

    struct
    {
        // current version of this image in the animation.  Initially points
        // to self.  For non-animated images, doesn't change.  Otherwise
        // when the animation flips over, it becomes cur->next.
        Image *current;

        // next image in the animation, or nullptr.
        Image *next;

        // tics before next anim change, or 0 if non-animated.
        uint16_t count;

        // animation speed (in tics), or 0 if non-animated.
        uint16_t speed;
    } animation_;

  public:
    Image();
    ~Image();

    float Right() const
    {
        return (float(actual_width_) / total_width_);
    }

    float Top() const
    {
        return (float(actual_height_) / total_height_);
    }

    float ScaledWidthActual() const
    {
        return (actual_width_ * scale_x_);
    }

    float ScaledHeightActual() const
    {
        return (actual_height_ * scale_y_);
    }

    float ScaledWidthTotal() const
    {
        return (total_width_ * scale_x_);
    }

    float ScaledHeightTotal() const
    {
        return (total_height_ * scale_y_);
    }

    float ScaledOffsetX() const
    {
        return (offset_x_ * scale_x_);
    }

    float ScaledOffsetY() const
    {
        return (offset_y_ * scale_y_);
    }
};

//
//  IMAGE LOOKUP
//
enum ImageLookupFlag
{
    kImageLookupNull  = 0x0001, // return nullptr rather than a dummy image
    kImageLookupExact = 0x0002, // type must be exactly the same
    kImageLookupNoNew = 0x0004, // image must already exist (don't create it)
    kImageLookupFont  = 0x0008, // font character (be careful with backups)
};

Image       *ImageContainerLookup(std::list<Image *> &bucket, const char *name, int source_type = -1);
const Image *ImageLookup(const char *name, ImageNamespace = kImageNamespaceGraphic, int flags = 0);

const Image *ImageForDummySprite(void);
const Image *ImageForDummySkin(void);
const Image *ImageForHomDetect(void);

//
//  IMAGE USAGE
//

extern int image_smoothing;

bool InitializeImages(void);
void AnimationTicker(void);
void DeleteAllImages(void);

const Image *CreatePackSprite(std::string packname, bool is_weapon);
void         CreateUserImages(void);
void         AnimateImageSet(const Image **images, int number, int speed);

void CreateFallbackFlat(void);
void CreateFallbackTexture(void);

GLuint ImageCache(const Image *image, bool anim = true, bool do_whiten = false);
void   ImagePrecache(const Image *image);

// this only needed during initialisation -- r_things.cpp
const Image **GetUserSprites(int *count);

int MakeValidTextureSize(int value);

enum ImageSource
{
    // Source was a graphic name
    kImageSourceGraphic = 0,

    // Source was a sprite name
    kImageSourceSprite,

    // INTERNAL ONLY: Source is from IMAGE.DDF
    kImageSourceUser,

    // INTERNAL ONLY: Source is dummy image
    kImageSourceDummy,
};

// Helper stuff for images in packages
extern std::list<Image *> real_graphics;
extern std::list<Image *> real_textures;
extern std::list<Image *> real_flats;
extern std::list<Image *> real_sprites;

Image *AddPackImageSmart(std::string_view name, ImageSource type, const std::string &packfile_name, std::list<Image *> &container,
                         const Image *replaces = nullptr);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
