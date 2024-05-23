// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 87be673333edb7d41bdefe2e1ec67c8bc997cac4 $
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

#include "res_texture.h"

#include <math.h>

#include <algorithm>
#include <unordered_map>

#include "cmdlib.h"
#include "i_system.h"
#include "m_fileio.h"
#include "m_memio.h"
#include "m_random.h"
#include "mud_includes.h"
#include "oscanner.h"
#include "r_sprites.h"
#include "r_state.h"
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "tables.h"
#include "v_video.h"
#include "w_wad.h"

// patch conversion structs
struct texpost_t
{
    uint8_t         row_off;
    std::vector<uint8_t> pixels;
};

struct texcolumn_t
{
    std::vector<texpost_t> posts;
};

TextureManager texturemanager;

void Res_InitTextureManager()
{
    texturemanager.startup();
}

void Res_ShutdownTextureManager()
{
    texturemanager.shutdown();
}

//
// Res_WarpTexture
//
// Alters the image in source_texture with a warping effect and saves the
// result in dest_texture.
//
static void Res_WarpTexture(Texture *dest_texture, const Texture *source_texture)
{
#if 0
	// [SL] Odamex experimental warping

	const palindex_t* source_buffer = source_texture->getData();

	int32_t widthbits = source_texture->getWidthBits();
	int32_t width = (1 << widthbits);
	int32_t widthmask = width - 1; 
	int32_t heightbits = source_texture->getHeightBits();
	int32_t height = (1 << heightbits);
	int32_t heightmask = height - 1;

	const int32_t time_offset = level.time * 50;

	for (int32_t x = 0; x < width; x++)
	{
		palindex_t* dest = dest_texture->getData() + (x << heightbits);

		for (int32_t y = 0; y < height; y++)
		{
			// calculate angle such that one sinusoidal period is 64 pixels.
			// add an offset based on the current time to create movement.
			int32_t angle_index = ((x << 7) + time_offset) & FINEMASK;

			// row and column offsets have the effect of stretching or
			// compressing the image according to a sine wave
			int32_t row_offset = (finesine[angle_index] << 2) >> FRACBITS;
			int32_t col_offset = (finesine[angle_index] << 1) >> FRACBITS;

			int32_t xindex = (((x + row_offset) & widthmask) << heightbits) + y;
			int32_t yindex = (x << heightbits) + ((y + col_offset) & heightmask);

			*dest++ = rt_blend2(source_buffer[xindex], 128, source_buffer[yindex], 127);
		}
	}
#endif

#if 0
	// [SL] ZDoom 1.22 warping

	const palindex_t* source_buffer = source_texture->getData();
	palindex_t* warped_buffer = dest_texture->getData();
	palindex_t temp_buffer[Texture::MAX_TEXTURE_HEIGHT];

	int32_t step = level.time * 32;
	for (int32_t y = height - 1; y >= 0; y--)
	{
		const byte* source = source_buffer + y;
		byte* dest = warped_buffer + y;

		int32_t xf = (finesine[(step + y * 128) & FINEMASK] >> 13) & widthmask;
		for (int32_t x = 0; x < width; x++)
		{
			*dest = source[xf << heightbits];
			dest += height;
			xf = (xf + 1) & widthmask;
		}
	}

	step = level.time * 23;
	for (int32_t x = width - 1; x >= 0; x--)
	{
		const byte *source = warped_buffer + (x << heightbits);
		byte *dest = temp_buffer;

		int32_t yf = (finesine[(step + 128 * (x + 17)) & FINEMASK] >> 13) & heightmask;
		for (int32_t y = 0; y < height; y++)
		{
			*dest++ = source[yf];
			yf = (yf + 1) & heightmask;
		}
		memcpy(warped_buffer + (x << heightbits), temp_buffer, height);
	}
#endif
}

// ============================================================================
//
// Texture
//
// ============================================================================

Texture::Texture() : mHandle(TextureManager::NO_TEXTURE_HANDLE)
{
    init(0, 0);
}

//
// Texture::init
//
// Sets up the member variable of Texture based on a supplied width and height.
//
void Texture::init(int32_t width, int32_t height)
{
    mWidth      = width;
    mHeight     = height;
    mFracHeight = height << FRACBITS;
    int32_t j = 0;
    for (j = 1; j * 2 <= width; j <<= 1)
            ;
    mWidthMask = j - 1;
    mOffsetX    = 0;
    mOffsetY    = 0;
    mScaleX     = FRACUNIT;
    mScaleY     = FRACUNIT;
    mData = NULL;
}

// ============================================================================
//
// TextureManager
//
// ============================================================================

TextureManager::TextureManager()
    : mHandleMap(2048)
{
}

TextureManager::~TextureManager()
{
}

//
// TextureManager::clear
//
// Frees all memory used by TextureManager, freeing all Texture objects
// and the supporting lookup structures.
//
void TextureManager::clear()
{
    // free normal textures
    for (HandleMap::iterator it = mHandleMap.begin(); it != mHandleMap.end(); ++it)
        if (it->second)
        {
           delete[] it->second->mData;
           delete it->second;
        }

    mHandleMap.clear();

    mAnimDefs.clear();

    // free warping original texture (not stored in mHandleMap)
    for (size_t i = 0; i < mWarpDefs.size(); i++)
        if (mWarpDefs[i].original_texture)
        {
            delete[] mWarpDefs[i].original_texture->mData;
            delete mWarpDefs[i].original_texture;
        }

    mWarpDefs.clear();
}

//
// TextureManager::startup
//
// Frees any existing textures and sets up the lookup structures for flats
// and wall textures. This should be called at the start of each map, prior
// to P_SetupLevel.
//
void TextureManager::startup()
{
    clear();

    // initialize the FLATS data
    char **rc = PHYSFS_enumerateFiles("flats");
    char **i;

    if (rc == NULL)
        I_FatalError("TextureManager::startup: No flats found in /flats!\n");

    for (i = rc; *i != NULL; i++)
    {
        std::string ext;
        if (!M_ExtractFileExtension(*i, ext))
            continue;
        if (stricmp(ext.c_str(), "png") != 0)
            continue;
        // Test PNG header - Dasho
        PHYSFS_File *pngcheck = PHYSFS_openRead(StrFormat("flats/%s", *i).c_str());
        if (pngcheck == NULL)
            continue;
        if (PHYSFS_fileLength(pngcheck) < 6) // not even big enough for PNG header
            continue;
        uint8_t header[6];
        if (PHYSFS_readBytes(pngcheck, header, 6) != 6)
        {
            PHYSFS_close(pngcheck);
            continue;
        }
        if (header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G' && header[4] == 0x0D &&
            header[5] == 0x0A)
        {
            // valid PNG header, add it
            std::string base;
            M_ExtractFileBase(*i, base);
            mFlatFilenames.push_back(StrFormat("flats/%s", *i));
            mEnumeratedFlatMap[OString(StdStringToUpper(base, 8))] = mFlatFilenames.size();
        }
        PHYSFS_close(pngcheck);
    }

    PHYSFS_freeList(rc);

    // initialize the TEXTURES data
    rc = PHYSFS_enumerateFiles("textures");
    i;

    if (rc == NULL)
        I_FatalError("TextureManager::startup: No textures found in /textures!\n");

    for (i = rc; *i != NULL; i++)
    {
        std::string ext;
        if (!M_ExtractFileExtension(*i, ext))
            continue;
        if (stricmp(ext.c_str(), "png") != 0)
            continue;
        // Test PNG header - Dasho
        PHYSFS_File *pngcheck = PHYSFS_openRead(StrFormat("textures/%s", *i).c_str());
        if (pngcheck == NULL)
            continue;
        if (PHYSFS_fileLength(pngcheck) < 6) // not even big enough for PNG header
            continue;
        uint8_t header[6];
        if (PHYSFS_readBytes(pngcheck, header, 6) != 6)
        {
            PHYSFS_close(pngcheck);
            continue;
        }
        if (header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G' && header[4] == 0x0D &&
            header[5] == 0x0A)
        {
            // valid PNG header, add it
            std::string base;
            M_ExtractFileBase(*i, base);
            mTextureFilenames.push_back(StrFormat("textures/%s", *i));
            mEnumeratedTextureMap[OString(StdStringToUpper(base, 8))] = mTextureFilenames.size();
        }
        PHYSFS_close(pngcheck);
    }

    PHYSFS_freeList(rc);

    // initialize the SRPITES data
	for (numsprites = 0; sprnames[numsprites]; numsprites++)
		;

	if (!numsprites)
		return;

	sprites = (spritedef_t *)Z_Malloc(numsprites * sizeof(*sprites), PU_STATIC, NULL);

    rc = PHYSFS_enumerateFiles("sprites");

    if (rc == NULL)
        I_FatalError("R_InitSpriteDefs: No sprites found in /sprites!\n");

    std::unordered_multimap<std::string, std::string> sprite_files;

    for (i = rc; *i != NULL; i++)
    {
        std::string base;
        M_ExtractFileBase(*i, base);
		// This requirement will eventually go away when we figure out how to handle
		// arbitrarily-long sprite names - Dasho
        if (!(base.size() == 6 || base.size() == 8))
            continue;
        StdStringToUpper(base);
        sprite_files.emplace(base.substr(0, 4), StrFormat("%s", *i));
    }

    PHYSFS_freeList(rc);

	for (int32_t i = 0; i < numsprites; i++)
	{
		memset(sprtemp, -1, sizeof(sprtemp));
		maxframe = -1;

		for (int32_t frame = 0; frame < MAX_SPRITE_FRAMES; frame++)
			for (int32_t r = 0; r < 8; r++)
				sprtemp[frame].texes[r] = TextureManager::NO_TEXTURE_HANDLE;

        std::string actor_id = sprnames[i];

        auto sprites = sprite_files.equal_range(actor_id);
        for (auto file = sprites.first; file != sprites.second; ++file)
        {
            std::string base;
            M_ExtractFileBase(file->second, base);
            StdStringToUpper(base);
            // Base should already be guaranteed 6 or 8 in length if we are here
            uint32_t frame = base[4] - 'A';
			uint32_t rotation = base[5] - '0';
            texturemanager.mSpriteFilenames.push_back(StrFormat("sprites/%s", file->second.c_str()));
			texhandle_t tex_id = texturemanager.mSpriteFilenames.size() | 0x00040000ul; // Sprite Handle Mask
            texturemanager.mEnumeratedSpriteMap[base] = tex_id;
			R_InstallSpriteTex(tex_id, frame, rotation, false);
            // can frame can be flipped?
            if (base.size() == 8)
            {
                frame = base[6] - 'A';
                rotation = base[7] - '0';
                R_InstallSpriteTex(tex_id, frame, rotation, true);
            }
        }

		R_InstallSprite(sprnames[i], i);
	}

    generateNoTexture();
    generateNotFoundTexture();

    if (clientside)
    {
        readAnimDefLump();
        readAnimatedLump();
    }
}

//
// TextureManager::shutdown
//
void TextureManager::shutdown()
{
    clear();
}

//
// TextureManager::precache
//
// Loads all of a level's textures into Zone memory.
// Requires that init() and P_SetupLevel be called first.
//
void TextureManager::precache()
{
#if 0
	// precache all the wall textures
	for (int32_t i = 0; i < numsides; i++)
	{
		if (sides[i].toptexture)
			getTexture(sides[i].toptexture);
		if (sides[i].midtexture)
			getTexture(sides[i].midtexture);
		if (sides[i].bottomtexture)
			getTexture(sides[i].bottomtexture);
	}

	// precache all the floor/ceiling textures
	for (int32_t i = 0; i < numsectors; i++)
	{
		getTexture(sectors[i].ceiling_texhandle);
		getTexture(sectors[i].floor_texhandle);
	}
#endif
}

//
// TextureManager::readAnimDefLump
//
// [RH] This uses a Hexen ANIMDEFS lump to define the animation sequences.
//
void TextureManager::readAnimDefLump()
{
    PHYSFS_File *rawinfo = PHYSFS_openRead("lumps/ANIMDEFS.txt");

    if (rawinfo == NULL)
        return;

    std::string buffer;
    buffer.resize(PHYSFS_fileLength(rawinfo));

    if (PHYSFS_readBytes(rawinfo, (void *)buffer.data(), buffer.size()) != buffer.size())
    {
        PHYSFS_close(rawinfo);
        return;
    }

    PHYSFS_close(rawinfo);

    OScannerConfig config = {
        "ANIMDEFS", // lumpName
        false,      // semiComments
        true,       // cComments
    };
    OScanner os = OScanner::openBuffer(config, buffer.data(), buffer.data() + buffer.size());

    while (os.scan())
    {
        if (os.compareToken("flat") || os.compareToken("texture"))
        {
            anim_t anim;

            Texture::TextureSourceType texture_type = Texture::TEX_TEXTURE;
            if (os.compareToken("flat"))
                texture_type = Texture::TEX_FLAT;

            os.mustScan();
            anim.basepic = texturemanager.getHandle(os.getToken(), texture_type);

            anim.curframe  = 0;
            anim.numframes = 0;
            memset(anim.speedmin, 1, anim_t::MAX_ANIM_FRAMES * sizeof(*anim.speedmin));
            memset(anim.speedmax, 1, anim_t::MAX_ANIM_FRAMES * sizeof(*anim.speedmax));

            while (os.scan())
            {
                if (!os.compareToken("pic"))
                {
                    os.unScan();
                    break;
                }

                if ((uint32_t)anim.numframes == anim_t::MAX_ANIM_FRAMES)
                    os.error("Animation has too many frames");

                uint8_t min = 1, max = 1;

                os.mustScanInt();
                const int32_t frame = os.getTokenInt();
                os.mustScan();
                if (os.compareToken("tics"))
                {
                    os.mustScanInt();
                    min = max = clamp(os.getTokenInt(), 0, 255);
                }
                else if (os.compareToken("rand"))
                {
                    os.mustScanInt();
                    min = MAX(os.getTokenInt(), 0);
                    os.mustScanInt();
                    max = MIN(os.getTokenInt(), 255);
                    if (min > max)
                        min = max = 1;
                }
                else
                {
                    os.error("Must specify a duration for animation frame");
                }

                anim.speedmin[anim.numframes] = min;
                anim.speedmax[anim.numframes] = max;
                anim.framepic[anim.numframes] = frame + anim.basepic - 1;
                anim.numframes++;
            }

            anim.countdown = anim.speedmin[0];

            if (anim.basepic != TextureManager::NOT_FOUND_TEXTURE_HANDLE &&
                anim.basepic != TextureManager::NO_TEXTURE_HANDLE)
                mAnimDefs.push_back(anim);
        }
        else if (os.compareToken("switch")) // Don't support switchdef yet...
        {
            // P_ProcessSwitchDef();
            // os.error("switchdef not supported.");
        }
        else if (os.compareToken("warp"))
        {
            os.mustScan();
            if (os.compareToken("flat") || os.compareToken("texture"))
            {
                Texture::TextureSourceType texture_type = Texture::TEX_TEXTURE;
                if (os.compareToken("flat"))
                    texture_type = Texture::TEX_FLAT;

                os.mustScan();

                const texhandle_t texhandle = texturemanager.getHandle(os.getToken(), texture_type);
                if (texhandle == TextureManager::NOT_FOUND_TEXTURE_HANDLE ||
                    texhandle == TextureManager::NO_TEXTURE_HANDLE)
                    continue;

                warp_t warp;

                // backup the original texture
                warp.original_texture = getTexture(texhandle);

                const int32_t width  = warp.original_texture->getWidth();
                const int32_t height = warp.original_texture->getHeight();

                // create a new texture of the same size for the warped image
                warp.warped_texture = createTexture(texhandle, width, height);

                mWarpDefs.push_back(warp);
            }
            else
            {
                os.error("Unknown error reading in ANIMDEFS");
            }
        }
    }
}

//
// TextureManager::readAnimatedLump
//
// Reads animation definitions from the ANIMATED lump.
//
// Load the table of animation definitions, checking for existence of
// the start and end of each frame. If the start doesn't exist the sequence
// is skipped, if the last doesn't exist, BOOM exits.
//
// Wall/Flat animation sequences, defined by name of first and last frame,
// The full animation sequence is given using all lumps between the start
// and end entry, in the order found in the WAD file.
//
// This routine modified to read its data from a predefined lump or
// PWAD lump called ANIMATED rather than a static table in this module to
// allow wad designers to insert or modify animation sequences.
//
// Lump format is an array of byte packed animdef_t structures, terminated
// by a structure with istexture == -1. The lump can be generated from a
// text source file using SWANTBLS.EXE, distributed with the BOOM utils.
// The standard list of switches and animations is contained in the example
// source text file DEFSWANI.DAT also in the BOOM util distribution.
//
// [RH] Rewritten to support BOOM ANIMATED lump but also make absolutely
// no assumptions about how the compiler packs the animdefs array.
//
void TextureManager::readAnimatedLump()
{
    uint8_t *filedata = NULL;
    int32_t filelen = M_ReadFile("lumps/ANIMATED.lmp", &filedata);

    if (filelen <= 0 || filedata == NULL)
        return;

    for (uint8_t *ptr = filedata; *ptr != 255; ptr += 23)
    {
        anim_t anim;

        Texture::TextureSourceType texture_type = *(ptr + 0) == 1 ? Texture::TEX_TEXTURE : Texture::TEX_FLAT;

        const char *startname = (const char *)(ptr + 10);
        const char *endname   = (const char *)(ptr + 1);

        texhandle_t start_texhandle = texturemanager.getHandle(startname, texture_type);
        texhandle_t end_texhandle   = texturemanager.getHandle(endname, texture_type);

        if (start_texhandle == TextureManager::NOT_FOUND_TEXTURE_HANDLE ||
            start_texhandle == TextureManager::NO_TEXTURE_HANDLE ||
            end_texhandle == TextureManager::NOT_FOUND_TEXTURE_HANDLE ||
            end_texhandle == TextureManager::NO_TEXTURE_HANDLE)
            continue;

        anim.basepic   = start_texhandle;
        anim.numframes = end_texhandle - start_texhandle + 1;

        if (anim.numframes <= 0)
            continue;
        anim.curframe = 0;

        int32_t speed      = LELONG(*(int32_t *)(ptr + 19));
        anim.countdown = speed - 1;

        for (int32_t i = 0; i < anim.numframes; i++)
        {
            anim.framepic[i] = anim.basepic + i;
            anim.speedmin[i] = anim.speedmax[i] = speed;
        }

        mAnimDefs.push_back(anim);
    }

    Z_Free(filedata);
}

//
// TextureManager::updateAnimatedTextures
//
// Handles ticking the animated textures and cyles the Textures within an
// animation definition.
//
void TextureManager::updateAnimatedTextures()
{
    if (!clientside)
        return;

    // cycle the animdef textures
    for (size_t i = 0; i < mAnimDefs.size(); i++)
    {
        anim_t *anim = &mAnimDefs[i];
        if (--anim->countdown == 0)
        {
            anim->curframe = (anim->curframe + 1) % anim->numframes;

            if (anim->speedmin[anim->curframe] == anim->speedmax[anim->curframe])
                anim->countdown = anim->speedmin[anim->curframe];
            else
                anim->countdown = M_Random() % (anim->speedmax[anim->curframe] - anim->speedmin[anim->curframe]) +
                                  anim->speedmin[anim->curframe];

            // cycle the Textures
            getTexture(anim->framepic[0]); // ensure Texture is still cached
            Texture *first_texture = mHandleMap[anim->framepic[0]];

            for (int32_t frame1 = 0; frame1 < anim->numframes - 1; frame1++)
            {
                int32_t frame2 = (frame1 + 1) % anim->numframes;
                getTexture(anim->framepic[frame2]); // ensure Texture is still cached
                mHandleMap[anim->framepic[frame1]] = mHandleMap[anim->framepic[frame2]];
            }

            mHandleMap[anim->framepic[anim->numframes - 1]] = first_texture;
        }
    }

    // warp textures
    for (size_t i = 0; i < mWarpDefs.size(); i++)
    {
        const Texture *original_texture = mWarpDefs[i].original_texture;
        Texture       *warped_texture   = mWarpDefs[i].warped_texture;
        Res_WarpTexture(warped_texture, original_texture);
    }
}

//
// TextureManager::generateNoTexture
//
// Generates an empty "texture" with dimensions 0x0 to prevent null access
// for certain functions
//
void TextureManager::generateNoTexture()
{
    const int32_t width = 0, height = 0;

    const texhandle_t handle  = NO_TEXTURE_HANDLE;
    Texture          *texture = createTexture(handle, width, height);
}

//
// TextureManager::generateNotFoundTexture
//
// Generates a checkerboard texture with 32x32 squares. This texture will be
// used whenever a texture is requested but not found in the WAD file.
//
void TextureManager::generateNotFoundTexture()
{
    const int32_t width = 64, height = 64;

    const texhandle_t handle  = NOT_FOUND_TEXTURE_HANDLE;
    Texture          *texture = createTexture(handle, width, height);

    if (clientside)
    {
        const argb_t     color1(0, 0, 255);   // blue
        const argb_t     color2(0, 255, 255); // yellow
        const palindex_t color1_index = V_BestColor(V_GetDefaultPalette()->basecolors, color1);
        const palindex_t color2_index = V_BestColor(V_GetDefaultPalette()->basecolors, color2);
        texture->mData = new uint8_t[width * height];

        for (int32_t x = 0; x < width / 2; x++)
        {
            memset(texture->mData + x * height + 0, color1_index, height / 2);
            memset(texture->mData + x * height + height / 2, color2_index, height / 2);
        }
        for (int32_t x = width / 2; x < width; x++)
        {
            memset(texture->mData + x * height + 0, color2_index, height / 2);
            memset(texture->mData + x * height + height / 2, color1_index, height / 2);
        }
    }
}

//
// TextureManager::createTexture
//
// Allocates memory for a new texture and returns a pointer to it. The texture
// is inserted into mHandlesMap for future retrieval.
//
Texture *TextureManager::createTexture(texhandle_t texhandle, int32_t width, int32_t height)
{
    width  = std::min<int32_t>(width, Texture::MAX_TEXTURE_WIDTH);
    height = std::min<int32_t>(height, Texture::MAX_TEXTURE_HEIGHT);

    Texture *texture = new Texture;
    texture->init(width, height);

    texture->mHandle = texhandle;

    mHandleMap.insert(HandleMapPair(texhandle, texture));

    return texture;
}

//
// TextureManager::freeTexture
//
// Frees the memory used by the specified texture and removes it
// from mHandlesMap.
//
void TextureManager::freeTexture(texhandle_t texhandle)
{
    if (texhandle == TextureManager::NOT_FOUND_TEXTURE_HANDLE || texhandle == TextureManager::NO_TEXTURE_HANDLE)
        return;

    HandleMap::iterator it = mHandleMap.find(texhandle);
    if (it != mHandleMap.end())
    {
        const Texture *texture = it->second;
        if (texture != NULL)
        {
            delete[] texture->mData;
            delete texture;
        }

        mHandleMap.erase(it);
    }
}

//
// TextureManager::getSpriteHandle
//
// Returns the handle for the sprite with the given name.
//
texhandle_t TextureManager::getSpriteHandle(const OString &name)
{
    EnumeratedSpriteMap::const_iterator it = mEnumeratedSpriteMap.find(name);
    if (it != mEnumeratedSpriteMap.end())
        return (texhandle_t)it->second | SPRITE_HANDLE_MASK;
    return NOT_FOUND_TEXTURE_HANDLE;
}

//
// TextureManger::cacheSprite
//
void TextureManager::cacheSprite(texhandle_t handle)
{
    uint32_t filenum = (handle & ~SPRITE_HANDLE_MASK);

    if (filenum-1 >= mSpriteFilenames.size())
            I_FatalError("TextureManager::cacheSprite: Invalid handle %d requested (%d is highest valid handle)\n", filenum-1, mSpriteFilenames.size()-1);

    PHYSFS_File *rawsprite = PHYSFS_openRead(mSpriteFilenames[filenum-1].c_str());

    if (rawsprite == NULL)
        I_FatalError("TextureManager::cacheSprite: Error opening %s\n", mSpriteFilenames[filenum-1].c_str());

    uint32_t filelen = PHYSFS_fileLength(rawsprite);

    uint8_t *filedata = new uint8_t[filelen];
    if (PHYSFS_readBytes(rawsprite, filedata, filelen) != filelen)
    {
        delete[] filedata;
        PHYSFS_close(rawsprite);
        I_FatalError("TexureManager::cacheSprite: Error reading %s\n", mSpriteFilenames[filenum-1].c_str());
    }

    int32_t height = 0;
    int32_t width = 0;
    int32_t bpp = 0;

    if (!stbi_info_from_memory(filedata, filelen, &width, &height, &bpp))
    {
        delete[] filedata;
        PHYSFS_close(rawsprite);
        I_FatalError("TexureManager::cacheSprite: %s is malformed!\n", mSpriteFilenames[filenum-1].c_str());
    }

    if (!clientside)
    {
        Texture *texture = createTexture(handle, width, height);
        delete[] filedata;
        PHYSFS_close(rawsprite);
    }
    else
    {
        int32_t need_bpp = 0;

        // if grayscale/paletted, convert to RGB/RGBA
        if (bpp == 1 || bpp == 2)
            need_bpp = bpp + 2;

        uint8_t *decoded_img = stbi_load_from_memory(filedata, filelen, &width, &height, &bpp, need_bpp);

        if (!decoded_img)
        {
            delete[] filedata;
            PHYSFS_close(rawsprite);
            I_FatalError("TexureManager::cacheSprite: Error decoding %s\n", mSpriteFilenames[filenum-1].c_str());
        }

        if (need_bpp)
            bpp = need_bpp;

        Texture *texture = createTexture(handle, width, height);

        // temporary PNG grAb chunk check
        int32_t i = 0;
        int32_t j = 0;
        for (; i<filelen && j<5; i++)
        {
            static uint8_t pgs[5] = { 0x08, 'g', 'r', 'A', 'b' };
            if (filedata[i] == pgs[j])
                j++;
            else
                j = 0;
        }
        if (j == 5)
        {
            int32_t x, y;
            memcpy(&x, &filedata[i], 4);
            i+=4;
            memcpy(&y, &filedata[i], 4);
            x = BELONG(x);
            y = BELONG(y);
            texture->setOffsetX(x);
            texture->setOffsetY(y);
        }

        delete[] filedata;

        // Convert image to column/post structure
        std::vector<texcolumn_t> columns;
        int32_t pixel_step = bpp * width;

        // Go through columns
        uint32_t offset = 0;
        for (int32_t c = 0; c < width; c++)
        {
            texcolumn_t col;
            texpost_t   post;
            post.row_off   = 0;
            bool ispost    = false;
            bool first_254 = true; // First 254 pixels should use absolute offsets
            uint8_t *pixel = decoded_img + c * bpp;

            offset          = c;
            uint8_t row_off = 0;
            for (int32_t r = 0; r < height; r++)
            {
                // For vanilla-compatible dimensions, use a split at 128 to prevent tiling.
                if (height < 256)
                {
                    if (row_off == 128)
                    {
                        // Finish current post if any
                        if (ispost)
                        {
                            col.posts.push_back(post);
                            post.pixels.clear();
                            ispost = false;
                        }
                    }
                }

                // Taller images cannot be expressed without tall patch support.
                // If we're at offset 254, create a dummy post for tall doom gfx support
                else if (row_off == 254)
                {
                    // Finish current post if any
                    if (ispost)
                    {
                        col.posts.push_back(post);
                        post.pixels.clear();
                        ispost = false;
                    }

                    // Begin relative offsets
                    first_254 = false;

                    // Create dummy post
                    post.row_off = 254;
                    col.posts.push_back(post);

                    // Clear post
                    row_off = 0;
                    ispost  = false;
                }

                argb_t color(bpp == 4 ? *(pixel+3) : 255, *pixel, *(pixel+1), *(pixel+2));

                // If the current pixel is not transparent, add it to the current post
                if (color.geta() != 0)
                {
                    // If we're not currently building a post, begin one and set its offset
                    if (!ispost)
                    {
                        // Set offset
                        post.row_off = row_off;

                        // Reset offset if we're in relative offsets mode
                        if (!first_254)
                            row_off = 0;

                        // Start post
                        ispost = true;
                    }

                    // Add the pixel to the post
                    post.pixels.push_back(V_BestColor(V_GetDefaultPalette()->basecolors, color));
                }
                else if (ispost)
                {
                    // If the current pixel is transparent and we are currently building
                    // a post, add the current post to the list and clear it
                    col.posts.push_back(post);
                    post.pixels.clear();
                    ispost = false;
                }

                // Go to next row
                offset += width;
                pixel += pixel_step;
                row_off++;
            }

            // If the column ended with a post, add it
            if (ispost)
                col.posts.push_back(post);

            // Add the column data
            columns.push_back(col);

            // Go to next column
            offset++;
        }

        stbi_image_free(decoded_img);
        PHYSFS_close(rawsprite);

        // Write doom gfx data to output
        MEMFILE *newpatch = mem_fopen_write();

        // Setup header
		mem_fwrite(&width, 2, 1, newpatch);
		mem_fwrite(&height, 2, 1, newpatch);
		mem_fwrite(&texture->mOffsetX, 2, 1, newpatch);
		mem_fwrite(&texture->mOffsetY, 2, 1, newpatch);

        // Write dummy column offsets for now
        std::vector<uint32_t> col_offsets(columns.size());
        mem_fwrite(col_offsets.data(), columns.size() * 4, 1, newpatch);

        // Write columns
        for (size_t c = 0; c < columns.size(); c++)
        {
            // Record column offset
            col_offsets[c] = mem_ftell(newpatch);

            // Determine column size (in bytes)
            uint32_t col_size = 0;
            for (auto& post : columns[c].posts)
                col_size += post.pixels.size() + 4;

            // Write column posts
            for (auto& post : columns[c].posts)
            {
                // Write row offset
                mem_fwrite(&post.row_off, 1, 1, newpatch);

                // Write no. of pixels
                uint8_t npix = post.pixels.size();
                mem_fwrite(&npix, 1, 1, newpatch);

                // Write unused byte
                uint8_t temp = (npix > 0) ? post.pixels[0] : 0;
                mem_fwrite(&temp, 1, 1, newpatch);

                // Write pixels
                for (auto& pixel : post.pixels)
                    mem_fwrite(&pixel, 1, 1, newpatch);

                // Write unused byte
                temp = (npix > 0) ? post.pixels.back() : 0;
                mem_fwrite(&temp, 1, 1, newpatch);
            }

            // Write '255' row to signal end of column
            uint8_t temp = 255;
            mem_fwrite(&temp, 1, 1, newpatch);
        }

        // Now we write column offsets
        mem_fseek(newpatch, 8, MEM_SEEK_SET);
        mem_fwrite(col_offsets.data(), columns.size() * 4, 1, newpatch);

        uint8_t *newnewpatch = new uint8_t[R_CalculateNewPatchSize((patch_t*)mem_fgetbuf(newpatch), mem_fsize(newpatch))+1];

        R_ConvertPatch((patch_t *)newnewpatch, (patch_t *)mem_fgetbuf(newpatch));

        mem_fclose(newpatch);

        texture->mData = newnewpatch;
    }
}

//
// TextureManager::getFlatHandle
//
// Returns the handle for the flat with the given name.
//
texhandle_t TextureManager::getFlatHandle(const OString &name)
{
    EnumeratedFlatMap::const_iterator it = mEnumeratedFlatMap.find(name);
    if (it != mEnumeratedFlatMap.end())
        return (texhandle_t)it->second | FLAT_HANDLE_MASK;
    return NOT_FOUND_TEXTURE_HANDLE;
}

//
// TextureManager::cacheFlat
//
// Loads a flat with the specified handle from the WAD file and composes
// a Texture object.
//
void TextureManager::cacheFlat(texhandle_t handle)
{
    uint32_t filenum = (handle & ~FLAT_HANDLE_MASK);

    if (filenum-1 >= mFlatFilenames.size())
            I_FatalError("TextureManager::cacheFlat: Invalid handle %d requested (%d is highest valid handle)\n", filenum-1, mFlatFilenames.size()-1);

    PHYSFS_File *rawflat = PHYSFS_openRead(mFlatFilenames[filenum-1].c_str());

    if (rawflat == NULL)
        I_FatalError("TextureManager::cacheFlat: Error opening %s\n", mFlatFilenames[filenum-1].c_str());

    uint32_t filelen = PHYSFS_fileLength(rawflat);

    uint8_t *filedata = new uint8_t[filelen];
    if (PHYSFS_readBytes(rawflat, filedata, filelen) != filelen)
    {
        delete[] filedata;
        PHYSFS_close(rawflat);
        I_FatalError("TexureManager::cacheFlat: Error reading %s\n", mFlatFilenames[filenum-1].c_str());
    }

    int32_t height = 0;
    int32_t width = 0;
    int32_t bpp = 0;
    int32_t need_bpp = 0;

    if (!stbi_info_from_memory(filedata, filelen, &width, &height, &bpp))
    {
        delete[] filedata;
        PHYSFS_close(rawflat);
        I_FatalError("TexureManager::cacheFlat: %s is malformed!\n", mFlatFilenames[filenum-1].c_str());
    }

    if (width != height)
    {
        delete[] filedata;
        PHYSFS_close(rawflat);
        I_FatalError("TexureManager::cacheFlat: %s is not square!\n", mFlatFilenames[filenum-1].c_str());
    }

    if (!clientside)
    {
        Texture *texture = createTexture(handle, width, height);
        delete[] filedata;
        PHYSFS_close(rawflat);
    }
    else
    {
        // if grayscale/paletted, convert to RGB/RGBA
        if (bpp == 1 || bpp == 2)
            need_bpp = bpp + 2;

        uint8_t *decoded_img = stbi_load_from_memory(filedata, filelen, &width, &height, &bpp, need_bpp);

        if (!decoded_img)
        {
            delete[] filedata;
            PHYSFS_close(rawflat);
            I_FatalError("TexureManager::cacheFlat: Error decoding %s\n", mFlatFilenames[filenum-1].c_str());
        }

        if (need_bpp)
            bpp = need_bpp;

        uint32_t pixel_step = width * bpp;

        Texture *texture = createTexture(handle, width, height);
        texture->mData = new uint8_t[width * height];
        memset(texture->mData, 0, width * height);

        for (uint32_t x = 0; x < width; x++)
        {
            uint8_t *dest = texture->mData + x;
            uint8_t *pixel = decoded_img + x * bpp;

            for (uint32_t y = 0; y < height; y++)
            {
                argb_t color(bpp == 4 ? *(pixel+3) : 255, *pixel, *(pixel+1), *(pixel+2));

                if (color.geta() != 0)
                    *dest = V_BestColor(V_GetDefaultPalette()->basecolors, color);

                dest += width;
                pixel += pixel_step;
            }
        }

        delete[] filedata;
        stbi_image_free(decoded_img);
        PHYSFS_close(rawflat);
    }
}

//
// TextureManager::getTextureHandle
//
// Returns the handle for the texture with the given name.
//
texhandle_t TextureManager::getTextureHandle(const OString &name)
{
    EnumeratedTextureMap::const_iterator it = mEnumeratedTextureMap.find(name);
    if (it != mEnumeratedTextureMap.end())
        return (texhandle_t)it->second | TEXTURE_HANDLE_MASK;
    return NOT_FOUND_TEXTURE_HANDLE;
}

//
// TextureManager::cacheTexture
//
// Composes a wall texture from a set of patches loaded from the WAD file.
//
void TextureManager::cacheTexture(texhandle_t handle)
{
    uint32_t filenum = (handle & ~TEXTURE_HANDLE_MASK);

    if (filenum-1 >= mTextureFilenames.size())
            I_FatalError("TextureManager::cacheTexture: Invalid handle %d requested (%d is highest valid handle)\n", filenum-1, mTextureFilenames.size()-1);

    PHYSFS_File *rawtex = PHYSFS_openRead(mTextureFilenames[filenum-1].c_str());

    if (rawtex == NULL)
        I_FatalError("TextureManager::cacheTexture: Error opening %s\n", mTextureFilenames[filenum-1].c_str());

    uint32_t filelen = PHYSFS_fileLength(rawtex);

    uint8_t *filedata = new uint8_t[filelen];
    if (PHYSFS_readBytes(rawtex, filedata, filelen) != filelen)
    {
        delete[] filedata;
        PHYSFS_close(rawtex);
        I_FatalError("TexureManager::cacheTexture: Error reading %s\n", mTextureFilenames[filenum-1].c_str());
    }

    int32_t height = 0;
    int32_t width = 0;
    int32_t bpp = 0;

    if (!stbi_info_from_memory(filedata, filelen, &width, &height, &bpp))
    {
        delete[] filedata;
        PHYSFS_close(rawtex);
        I_FatalError("TexureManager::cacheTexture: %s is malformed!\n", mTextureFilenames[filenum-1].c_str());
    }

    if (!clientside)
    {
        Texture *texture = createTexture(handle, width, height);
        delete[] filedata;
        PHYSFS_close(rawtex);
    }
    else
    {
        int32_t need_bpp = 0;

        // if grayscale/paletted, convert to RGB/RGBA
        if (bpp == 1 || bpp == 2)
            need_bpp = bpp + 2;

        uint8_t *decoded_img = stbi_load_from_memory(filedata, filelen, &width, &height, &bpp, need_bpp);

        if (!decoded_img)
        {
            delete[] filedata;
            PHYSFS_close(rawtex);
            I_FatalError("TexureManager::cacheTexture: Error decoding %s\n", mTextureFilenames[filenum-1].c_str());
        }

        if (need_bpp)
            bpp = need_bpp;

        Texture *texture = createTexture(handle, width, height);

        // temporary PNG grAb chunk check
        int32_t i = 0;
        int32_t j = 0;
        for (; i<filelen && j<5; i++)
        {
            static uint8_t pgs[5] = { 0x08, 'g', 'r', 'A', 'b' };
            if (filedata[i] == pgs[j])
                j++;
            else
                j = 0;
        }
        if (j == 5)
        {
            int32_t x, y;
            memcpy(&x, &filedata[i], 4);
            i+=4;
            memcpy(&y, &filedata[i], 4);
            x = BELONG(x);
            y = BELONG(y);
            texture->setOffsetX(x);
            texture->setOffsetY(y);
        }

        delete[] filedata;

        // Convert image to column/post structure
        std::vector<texcolumn_t> columns;
        int32_t pixel_step = bpp * width;

        // Go through columns
        uint32_t offset = 0;
        for (int32_t c = 0; c < width; c++)
        {
            texcolumn_t col;
            texpost_t   post;
            post.row_off   = 0;
            bool ispost    = false;
            bool first_254 = true; // First 254 pixels should use absolute offsets
            uint8_t *pixel = decoded_img + c * bpp;

            offset          = c;
            uint8_t row_off = 0;
            for (int32_t r = 0; r < height; r++)
            {
                // For vanilla-compatible dimensions, use a split at 128 to prevent tiling.
                if (height < 256)
                {
                    if (row_off == 128)
                    {
                        // Finish current post if any
                        if (ispost)
                        {
                            col.posts.push_back(post);
                            post.pixels.clear();
                            ispost = false;
                        }
                    }
                }

                // Taller images cannot be expressed without tall patch support.
                // If we're at offset 254, create a dummy post for tall doom gfx support
                else if (row_off == 254)
                {
                    // Finish current post if any
                    if (ispost)
                    {
                        col.posts.push_back(post);
                        post.pixels.clear();
                        ispost = false;
                    }

                    // Begin relative offsets
                    first_254 = false;

                    // Create dummy post
                    post.row_off = 254;
                    col.posts.push_back(post);

                    // Clear post
                    row_off = 0;
                    ispost  = false;
                }

                argb_t color(bpp == 4 ? *(pixel+3) : 255, *pixel, *(pixel+1), *(pixel+2));

                // If the current pixel is not transparent, add it to the current post
                if (color.geta() != 0)
                {
                    // If we're not currently building a post, begin one and set its offset
                    if (!ispost)
                    {
                        // Set offset
                        post.row_off = row_off;

                        // Reset offset if we're in relative offsets mode
                        if (!first_254)
                            row_off = 0;

                        // Start post
                        ispost = true;
                    }

                    // Add the pixel to the post
                    post.pixels.push_back(V_BestColor(V_GetDefaultPalette()->basecolors, color));
                }
                else if (ispost)
                {
                    // If the current pixel is transparent and we are currently building
                    // a post, add the current post to the list and clear it
                    col.posts.push_back(post);
                    post.pixels.clear();
                    ispost = false;
                }

                // Go to next row
                offset += width;
                pixel += pixel_step;
                row_off++;
            }

            // If the column ended with a post, add it
            if (ispost)
                col.posts.push_back(post);

            // Add the column data
            columns.push_back(col);

            // Go to next column
            offset++;
        }

        stbi_image_free(decoded_img);
        PHYSFS_close(rawtex);

        // Write doom gfx data to output
        MEMFILE *newpatch = mem_fopen_write();

        // Setup header
		mem_fwrite(&width, 2, 1, newpatch);
		mem_fwrite(&height, 2, 1, newpatch);
		mem_fwrite(&texture->mOffsetX, 2, 1, newpatch);
		mem_fwrite(&texture->mOffsetY, 2, 1, newpatch);

        // Write dummy column offsets for now
        std::vector<uint32_t> col_offsets(columns.size());
        mem_fwrite(col_offsets.data(), columns.size() * 4, 1, newpatch);

        // Write columns
        for (size_t c = 0; c < columns.size(); c++)
        {
            // Record column offset
            col_offsets[c] = mem_ftell(newpatch);

            // Determine column size (in bytes)
            uint32_t col_size = 0;
            for (auto& post : columns[c].posts)
                col_size += post.pixels.size() + 4;

            // Write column posts
            for (auto& post : columns[c].posts)
            {
                // Write row offset
                mem_fwrite(&post.row_off, 1, 1, newpatch);

                // Write no. of pixels
                uint8_t npix = post.pixels.size();
                mem_fwrite(&npix, 1, 1, newpatch);

                // Write unused byte
                uint8_t temp = (npix > 0) ? post.pixels[0] : 0;
                mem_fwrite(&temp, 1, 1, newpatch);

                // Write pixels
                for (auto& pixel : post.pixels)
                    mem_fwrite(&pixel, 1, 1, newpatch);

                // Write unused byte
                temp = (npix > 0) ? post.pixels.back() : 0;
                mem_fwrite(&temp, 1, 1, newpatch);
            }

            // Write '255' row to signal end of column
            uint8_t temp = 255;
            mem_fwrite(&temp, 1, 1, newpatch);
        }

        // Now we write column offsets
        mem_fseek(newpatch, 8, MEM_SEEK_SET);
        mem_fwrite(col_offsets.data(), columns.size() * 4, 1, newpatch);

        uint8_t *newnewpatch = new uint8_t[R_CalculateNewPatchSize((patch_t*)mem_fgetbuf(newpatch), mem_fsize(newpatch))+1];

        R_ConvertPatch((patch_t *)newnewpatch, (patch_t *)mem_fgetbuf(newpatch));

        mem_fclose(newpatch);

        texture->mData = newnewpatch;
    }
}

//
// TextureManager::getHandle
//
// Returns the handle for the texture that matches the supplied name.
//
texhandle_t TextureManager::getHandle(const OString &name, Texture::TextureSourceType type)
{
    OString uname(StdStringToUpper(name));

    // sidedefs with the '-' texture indicate there should be no texture used
    if (uname[0] == '-' && type == Texture::TEX_TEXTURE)
        return NO_TEXTURE_HANDLE;

    texhandle_t handle = NOT_FOUND_TEXTURE_HANDLE;

    // check for the texture in the default location specified by type
    if (type == Texture::TEX_FLAT)
        handle = getFlatHandle(uname);
    else if (type == Texture::TEX_TEXTURE)
        handle = getTextureHandle(uname);
    else if (type == Texture::TEX_SPRITE)
        handle = getSpriteHandle(uname);

    return handle;
}

//
// TextureManager::getHandle
//
// Returns the handle for the texture that matches the supplied name.
// [SL] This version will accept WAD lump names that are not properly
// zero terminated (max 8 characters).
//
texhandle_t TextureManager::getHandle(const char *name, Texture::TextureSourceType type)
{
    OString uname(StdStringToUpper(name, 8));
    return getHandle(uname, type);
}

//
// TextureManager::getTexture
//
// Returns the Texture for the appropriate handle. If the Texture is not
// currently cached, it will be loaded from the disk and cached.
//
const Texture *TextureManager::getTexture(texhandle_t handle)
{
    Texture *texture = mHandleMap[handle];
    if (!texture)
    {
        if (handle & FLAT_HANDLE_MASK)
            cacheFlat(handle);
        else if (handle & TEXTURE_HANDLE_MASK)
            cacheTexture(handle);
        else if (handle & SPRITE_HANDLE_MASK)
            cacheSprite(handle);

        texture = mHandleMap[handle];
    }

    return texture;
}

VERSION_CONTROL(res_texture_cpp, "$Id: 87be673333edb7d41bdefe2e1ec67c8bc997cac4 $")
