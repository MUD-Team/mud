// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 1a869a3bb3dfd4e1f71251b9f605f97723b20ce8 $
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
//
// Manager for texture resource loading and converting.
//
//-----------------------------------------------------------------------------

#pragma once

#include <vector>

#include "hashtable.h"
#include "m_fixed.h"
#include "m_ostring.h"

typedef uint32_t texhandle_t;

class Texture;
class TextureManager;

void Res_InitTextureManager();
void Res_ShutdownTextureManager();

// ============================================================================
//
// Texture
//
// ============================================================================
//
// Texture is a unified abstraction of Doom's various graphic formats.
// If the image is a flat, it is stored in row-major format as a set of
// 8-bit palettized pixels. If it is a texture or sprite, it is stored as
// a patch_t struct utilizing tallposts.
//
// Sprites and textures are treated as being comprised of a singular patch;
// for textures this differs from traditional Doom format where a texture
// could be a composite image derived from multiple patches.
//

class Texture
{
  public:
    typedef enum
    {
        TEX_FLAT,
        TEX_SPRITE,
        TEX_TEXTURE
    } TextureSourceType;

    static const uint32_t MAX_TEXTURE_WIDTH  = 2048;
    static const uint32_t MAX_TEXTURE_HEIGHT = 2048;

    texhandle_t getHandle() const
    {
        return mHandle;
    }

    uint8_t *getData() const
    {
        return mData;
    }

    int32_t getWidth() const
    {
        return mWidth;
    }

    int32_t getHeight() const
    {
        return mHeight;
    }

    uint32_t getWidthBits() const
    {
        return mWidthBits;
    }

    uint32_t getHeightBits() const
    {
        return mHeightBits;
    }

    fixed_t getFracHeight() const
    {
        return mFracHeight;
    }

    int32_t getWidthMask() const
    {
        return mWidthMask;
    }

    int32_t getOffsetX() const
    {
        return mOffsetX;
    }

    int32_t getOffsetY() const
    {
        return mOffsetY;
    }

    fixed_t getScaleX() const
    {
        return mScaleX;
    }

    fixed_t getScaleY() const
    {
        return mScaleY;
    }

    void setOffsetX(int32_t value)
    {
        mOffsetX = value;
    }

    void setOffsetY(int32_t value)
    {
        mOffsetY = value;
    }

  private:
    friend class TextureManager;

    Texture();

    void init(int32_t width, int32_t height);

    texhandle_t mHandle;

    fixed_t mScaleX;
    fixed_t mScaleY;

    uint16_t mWidth;
    uint16_t mHeight;

    fixed_t mFracHeight;
    int32_t mWidthMask;

    int16_t mOffsetX;
    int16_t mOffsetY;

    uint8_t mWidthBits;
    uint8_t mHeightBits;

    TextureSourceType mType;

    // indexed data (flats) or patch data (walls/sprites)
    uint8_t *mData;
};

// ============================================================================
//
// TextureManager
//
// ============================================================================
//
// TextureManager provides a unified interface for loading and accessing the
// various types of graphic formats needed by Doom's renderer and interface.
//
// A handle to a texture is retrieved by calling getHandle() with the name
// of the texture and a texture source type. The texture source
// type parameter is used to indicate where to search for the
// texture.
//
// A pointer to a Texture object can be retrieved by passing a texture
// handle to getTexture().
//
class TextureManager
{
  public:
    TextureManager();
    ~TextureManager();

    void startup();
    void shutdown();

    void precache();

    void updateAnimatedTextures();

    texhandle_t    getHandle(const char *name, Texture::TextureSourceType type);
    texhandle_t    getHandle(const OString &name, Texture::TextureSourceType type);
    const Texture *getTexture(texhandle_t handle);

    Texture *createTexture(texhandle_t handle, Texture::TextureSourceType type, int32_t width, int32_t height);
    void     freeTexture(texhandle_t handle);

    static const texhandle_t NO_TEXTURE_HANDLE        = 0x0;
    static const texhandle_t NOT_FOUND_TEXTURE_HANDLE = 0x1;

  private:
    static const uint32_t FLAT_HANDLE_MASK    = 0x00020000ul;
    static const uint32_t SPRITE_HANDLE_MASK  = 0x00040000ul;
    static const uint32_t TEXTURE_HANDLE_MASK = 0x00080000ul;

    // initialization routines
    void clear();
    void generateNoTexture();
    void generateNotFoundTexture();
    void readAnimDefLump();
    void readAnimatedLump();

    void remapFlat(Texture *texture, uint8_t *argbData);
    void generateColumns(Texture *texture, uint8_t *argbData);

    // sprites
    texhandle_t                           getSpriteHandle(const OString &name);
    void                                  cacheSprite(texhandle_t handle);
    typedef OHashTable<OString, uint32_t> EnumeratedSpriteMap;
    EnumeratedSpriteMap                   mEnumeratedSpriteMap;
    std::vector<std::string>              mSpriteFilenames;

    // flats
    texhandle_t                           getFlatHandle(const OString &name);
    void                                  cacheFlat(texhandle_t handle);
    typedef OHashTable<OString, uint32_t> EnumeratedFlatMap;
    EnumeratedFlatMap                     mEnumeratedFlatMap;
    std::vector<std::string>              mFlatFilenames;

    // wall textures
    texhandle_t                           getTextureHandle(const OString &name);
    void                                  cacheTexture(texhandle_t handle);
    typedef OHashTable<OString, uint32_t> EnumeratedTextureMap;
    EnumeratedTextureMap                  mEnumeratedTextureMap;
    std::vector<std::string>              mTextureFilenames;

    // maps texture handles to Texture*
    typedef OHashTable<texhandle_t, Texture *> HandleMap;
    typedef std::pair<texhandle_t, Texture *>  HandleMapPair;
    HandleMap                                  mHandleMap;

    // animated textures
    struct anim_t
    {
        static const uint32_t MAX_ANIM_FRAMES = 32;
        texhandle_t           basepic;
        int16_t               numframes;
        uint8_t               countdown;
        uint8_t               curframe;
        uint8_t               speedmin[MAX_ANIM_FRAMES];
        uint8_t               speedmax[MAX_ANIM_FRAMES];
        texhandle_t           framepic[MAX_ANIM_FRAMES];
    };

    std::vector<anim_t> mAnimDefs;

    // warped textures
    struct warp_t
    {
        const Texture *original_texture;
        Texture       *warped_texture;
    };

    std::vector<warp_t> mWarpDefs;
};

extern TextureManager texturemanager;
