// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
//
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
//  UIRender header.
//
//-----------------------------------------------------------------------------

#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include "../../sdl/i_sdl.h"
#include "v_pixelformat.h"

class IRenderSurface;

class ISDL20Window;

class UIRenderInterface : public Rml::RenderInterface
{
  public:
    UIRenderInterface();
    virtual ~UIRenderInterface();

    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                Rml::Span<const int32_t>     indices) override;
    void                        ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;
    void                        RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation,
                                               Rml::TextureHandle texture) override;

    Rml::TextureHandle LoadTexture(Rml::Vector2i &texture_dimensions, const Rml::String &source) override;
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
    void               ReleaseTexture(Rml::TextureHandle texture_handle) override;

    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;

    void setMode(uint16_t width, uint16_t height, const PixelFormat *format, ISDL20Window *window, bool vsync,
                 const char *render_scale_quality);

    void BeginFrame();
    void EndFrame();

    void setRenderSurface(IRenderSurface *surface);

    SDL_Texture *getPlayerViewTexture() const
    {
        return mPlayerViewTexture;
    }

    SDL_Renderer *getRenderer() const
    {
        return mRenderer;
    }

    PixelFormat GetPixelFormat() const
    {
        return mFormat;
    }

    uint16_t getWidth() const
    {
        return mWidth;
    }

    uint16_t getHeight() const
    {
        return mHeight;
    }

    ISDL20Window *getSDLWindow()
    {
        return mWindow;
    }

    SDL_Renderer *getSDLRenderer()
    {
        return mRenderer;
    }

  private:
    struct GeometryView
    {
        Rml::Span<const Rml::Vertex> vertices;
        Rml::Span<const int32_t>     indices;
    };

    // Rendering surface
    IRenderSurface *mRenderSurface = nullptr;

    SDL_Texture *mPlayerViewTexture = nullptr;

    SDL_Renderer *mRenderer             = nullptr;
    SDL_BlendMode mBlendMode            = {};
    SDL_Rect      mRectScissor          = {};
    bool          mScissorRegionEnabled = false;

    ISDL20Window *mWindow = nullptr;
    uint16_t      mWidth;
    uint16_t      mHeight;
    bool          mVSync;
    PixelFormat   mFormat;
    Uint32        mSDLDisplayFormat = 0;
};
