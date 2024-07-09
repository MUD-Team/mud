//----------------------------------------------------------------------------
//  EDGE Colour Code
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

#include "r_colormap.h"

#include "ddf_colormap.h"
#include "ddf_game.h"
#include "ddf_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_main.h"
#include "e_player.h"
#include "epi.h"
#include "epi_str_util.h"
#include "g_game.h" // current_map
#include "i_defs_gl.h"
#include "i_system.h"
#include "m_argv.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_shader.h"
#include "r_texgl.h"
#include "r_units.h"
#include "w_files.h"
#include "w_wad.h"

extern ConsoleVariable force_flat_lighting;

EDGE_DEFINE_CONSOLE_VARIABLE(sector_brightness_correction, "5", kConsoleVariableFlagArchive)

void TransformColourmap(Colormap *colmap)
{
    if (colmap->font_colour_ == kRGBANoValue)
    {
        if (colmap->gl_color_ != kRGBANoValue)
            colmap->font_colour_ = colmap->gl_color_;
    }
}

void GetColormapRGB(const Colormap *colmap, float *r, float *g, float *b)
{
    if (colmap->gl_color_ != kRGBANoValue)
    {
        RGBAColor col = colmap->gl_color_;
        (*r) = ((col >> 24) & 0xFF) / 255.0f;
        (*g) = ((col >> 16) & 0xFF) / 255.0f;
        (*b) = ((col >> 8) & 0xFF) / 255.0f;
    }
}

RGBAColor GetFontColor(const Colormap *colmap)
{
    if (!colmap)
        return kRGBANoValue;

    if (colmap->font_colour_ == kRGBANoValue)
    {
        // Intention Const Override
        TransformColourmap((Colormap *)colmap);
    }

    return colmap->font_colour_;
}

RGBAColor ParseFontColor(const char *name, bool strict)
{
    if (!name || !name[0])
        return kRGBANoValue;

    RGBAColor rgb;

    if (name[0] == '#')
    {
        int r, g, b;

        if (sscanf(name, " #%2x%2x%2x ", &r, &g, &b) != 3)
            FatalError("Bad RGB colour value: %s\n", name);

        rgb = epi::MakeRGBA((uint8_t)r, (uint8_t)g, (uint8_t)b);
    }
    else
    {
        const Colormap *colmap = colormaps.Lookup(name);

        if (!colmap)
        {
            if (strict)
                FatalError("Unknown colormap: '%s'\n", name);
            else
                LogDebug("Unknown colormap: '%s'\n", name);

            return SG_MAGENTA_RGBA32;
        }

        rgb = GetFontColor(colmap);
    }

    if (rgb == kRGBANoValue)
        rgb ^= 0x00010100;

    return rgb;
}

//----------------------------------------------------------------------------
//  COLORMAP SHADERS
//----------------------------------------------------------------------------

static int DoomLightingEquation(int L, float dist)
{
    /* L in the range 0 to 63 */

    int min_L = HMM_Clamp(0, 36 - L, 31);

    int index = (59 - L) - int(1280 / HMM_MAX(1, dist));

    /* result is colormap index (0 bright .. 31 dark) */
    return HMM_Clamp(min_L, index, 31);
}

class ColormapShader : public AbstractShader
{
  private:
    const Colormap *colormap_;

    int light_level_;

    GLuint fade_texture_;

    LightingModel lighting_model_;

    RGBAColor whites_[32];

    RGBAColor fog_color_;
    float     fog_density_;

    // for DDFLEVL fog checks
    Sector *sector_;

  public:
    ColormapShader(const Colormap *CM)
        : colormap_(CM), light_level_(255), fade_texture_(0), lighting_model_(kLightingModelDoom),
          fog_color_(kRGBANoValue), fog_density_(0), sector_(nullptr)
    {
    }

    virtual ~ColormapShader()
    {
        DeleteTex();
    }

  private:
    inline float DistanceFromViewPlane(float x, float y, float z)
    {
        float dx = (x - view_x) * view_forward.X;
        float dy = (y - view_y) * view_forward.Y;
        float dz = (z - view_z) * view_forward.Z;

        return dx + dy + dz;
    }

    inline void TextureCoordinates(RendererVertex *v, int t, const HMM_Vec3 *lit_pos)
    {
        float dist = DistanceFromViewPlane(lit_pos->X, lit_pos->Y, lit_pos->Z);

        int L = light_level_ / 4; // need integer range 0-63

        v->texture_coordinates[t].X = dist / 1600.0;
        v->texture_coordinates[t].Y = (L + 0.5) / 64.0;
    }

  public:
    virtual void Sample(ColorMixer *col, float x, float y, float z)
    {
        // FIXME: assumes standard COLORMAP

        float dist = DistanceFromViewPlane(x, y, z);

        int cmap_idx;

        if (lighting_model_ >= kLightingModelFlat)
            cmap_idx = HMM_Clamp(0, 42 - light_level_ / 6, 31);
        else
            cmap_idx = DoomLightingEquation(light_level_ / 4, dist);

        RGBAColor WH = whites_[cmap_idx];

        col->modulate_red_ += epi::GetRGBARed(WH);
        col->modulate_green_ += epi::GetRGBAGreen(WH);
        col->modulate_blue_ += epi::GetRGBABlue(WH);

        // FIXME: for foggy maps, need to adjust add_red_/G/B too
    }

    virtual void Corner(ColorMixer *col, float nx, float ny, float nz, MapObject *mod_pos, bool is_weapon)
    {
        // TODO: improve this (normal-ise a little bit)

        float mx = mod_pos->x;
        float my = mod_pos->y;
        float mz = mod_pos->z + mod_pos->height_ / 2;

        if (is_weapon)
        {
            mx += view_cosine * 110;
            my += view_sine * 110;
        }

        Sample(col, mx, my, mz);
    }

    virtual void WorldMix(GLuint shape, int num_vert, GLuint tex, float alpha, int *pass_var, int blending, bool masked,
                          void *data, ShaderCoordinateFunction func)
    {
        RGBAColor fc_to_use = fog_color_;
        float     fd_to_use = fog_density_;
        // check for DDFLEVL fog
        if (fc_to_use == kRGBANoValue)
        {
            if (EDGE_IMAGE_IS_SKY(sector_->ceiling))
            {
                fc_to_use = current_map->outdoor_fog_color_;
                fd_to_use = 0.01f * current_map->outdoor_fog_density_;
            }
            else
            {
                fc_to_use = current_map->indoor_fog_color_;
                fd_to_use = 0.01f * current_map->indoor_fog_density_;
            }
        }

        RendererVertex *glvert = BeginRenderUnit(shape, num_vert, GL_MODULATE, tex, GL_MODULATE, fade_texture_,
                                                   *pass_var, blending, fc_to_use, fd_to_use);

        for (int v_idx = 0; v_idx < num_vert; v_idx++)
        {
            RendererVertex *dest = glvert + v_idx;

            dest->rgba_color[3] = alpha;

            HMM_Vec3 lit_pos;

            (*func)(data, v_idx, &dest->position, dest->rgba_color, &dest->texture_coordinates[0], &dest->normal,
                    &lit_pos);

            TextureCoordinates(dest, 1, &lit_pos);
        }

        EndRenderUnit(num_vert);

        (*pass_var) += 1;
    }

  private:
    void MakeColormapTexture()
    {
        ImageData img(256, 64, 4);

        if (colormap_) // GL_COLOUR
        {
            for (int ci = 0; ci < 32; ci++)
            {
                int r = epi::GetRGBARed(colormap_->gl_color_) * (31 - ci) / 31;
                int g = epi::GetRGBAGreen(colormap_->gl_color_) * (31 - ci) / 31;
                int b = epi::GetRGBABlue(colormap_->gl_color_) * (31 - ci) / 31;

                whites_[ci] = epi::MakeRGBA(r, g, b);
            }
        }
        else
        {
            for (int ci = 0; ci < 32; ci++)
            {
                int ity = 255 - ci * 8 - ci / 5;

                whites_[ci] = epi::MakeRGBA(ity, ity, ity);
            }
        }

        for (int L = 0; L < 64; L++)
        {
            uint8_t *dest = img.PixelAt(0, L);

            for (int x = 0; x < 256; x++, dest += 4)
            {
                float dist = 1600.0f * x / 255.0;

                int index;

                if (lighting_model_ >= kLightingModelFlat)
                {
                    // FLAT lighting
                    index = HMM_Clamp(0, 42 - (L * 2 / 3), 31);
                }
                else
                {
                    // DOOM lighting formula
                    index = DoomLightingEquation(L, dist);
                }

                // GL_MODULATE mode
                if (colormap_)
                {
                    dest[0] = epi::GetRGBARed(whites_[index]);
                    dest[1] = epi::GetRGBAGreen(whites_[index]);
                    dest[2] = epi::GetRGBABlue(whites_[index]);
                    dest[3] = 255;
                }
                else
                {
                    dest[0] = 255 - index * 8;
                    dest[1] = dest[0];
                    dest[2] = dest[0];
                    dest[3] = 255;
                }
            }
        }

        fade_texture_ = UploadTexture(&img, kUploadSmooth | kUploadClamp);
    }

  public:
    void Update()
    {
        if (fade_texture_ == 0 || (force_flat_lighting.d_ && lighting_model_ != kLightingModelFlat) ||
            (!force_flat_lighting.d_ && lighting_model_ != current_map->episode_->lighting_))
        {
            if (fade_texture_ != 0)
            {
                glDeleteTextures(1, &fade_texture_);
            }

            if (force_flat_lighting.d_)
                lighting_model_ = kLightingModelFlat;
            else
                lighting_model_ = current_map->episode_->lighting_;

            MakeColormapTexture();
        }
    }

    void DeleteTex()
    {
        if (fade_texture_ != 0)
        {
            glDeleteTextures(1, &fade_texture_);
            fade_texture_ = 0;
        }
    }

    void SetLight(int level)
    {
        light_level_ = level;
    }

    void SetFog(RGBAColor fog_color, float fog_density)
    {
        fog_color_   = fog_color;
        fog_density_ = fog_density;
    }

    void SetSector(Sector *sec)
    {
        sector_ = sec;
    }
};

static ColormapShader *standard_colormap_shader;

AbstractShader *GetColormapShader(const struct RegionProperties *props, int light_add, Sector *sec)
{
    if (!standard_colormap_shader)
        standard_colormap_shader = new ColormapShader(nullptr);

    ColormapShader *shader = standard_colormap_shader;

    if (props->colourmap)
    {
        if (props->colourmap->analysis_)
            shader = (ColormapShader *)props->colourmap->analysis_;
        else
        {
            shader = new ColormapShader(props->colourmap);

            // Intentional Const Override
            Colormap *CM  = (Colormap *)props->colourmap;
            CM->analysis_ = shader;
        }
    }

    EPI_ASSERT(shader);

    shader->Update();

    int lit_Nom = props->light_level + light_add + ((sector_brightness_correction.d_ - 5) * 10);

    if (!(props->colourmap && (props->colourmap->special_ & kColorSpecialNoFlash)) || render_view_extra_light > 250)
    {
        lit_Nom += render_view_extra_light;
    }

    lit_Nom = HMM_Clamp(0, lit_Nom, 255);

    shader->SetLight(lit_Nom);

    shader->SetFog(props->fog_color, props->fog_density);

    shader->SetSector(sec);

    return shader;
}

void DeleteColourmapTextures(void)
{
    if (standard_colormap_shader)
        standard_colormap_shader->DeleteTex();

    standard_colormap_shader = nullptr;

    for (Colormap *cmap : colormaps)
    {
        if (cmap->analysis_)
        {
            ColormapShader *shader = (ColormapShader *)cmap->analysis_;

            shader->DeleteTex();
        }
    }
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
