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
//  UIRender module.
//
//-----------------------------------------------------------------------------

#include "ui_render.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Types.h>
#include <stb_image.h>

#include "../sdl/i_video.h"
#include "../sdl/i_video_sdl20.h"
#include "i_system.h"
#include "r_main.h"

#include"ui_main.h"

UIRenderInterface::UIRenderInterface()
{
    // RmlUi serves vertex colors and textures with premultiplied alpha, set the blend mode accordingly.
    // Equivalent to glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA).
    mBlendMode =
        SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
                                   SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);

    Rml::SetRenderInterface(this);
}

UIRenderInterface::~UIRenderInterface()
{
    void UI_IMGUI_Shutdown();
    UI_IMGUI_Shutdown();
}

void UIRenderInterface::BeginFrame()
{
    if (!mRenderer)
    {
        return;
    }

    UI::processEvents();

    SDL_RenderSetViewport(mRenderer, nullptr);
    SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
    SDL_RenderClear(mRenderer);
    SDL_SetRenderDrawBlendMode(mRenderer, mBlendMode);

    void UI_IMGUI_BeginFrame();
    UI_IMGUI_BeginFrame();
}

void UIRenderInterface::EndFrame()
{
    if (!mRenderer)
    {
        return;
    }

    void UI_IMGUI_EndFrame();
    UI_IMGUI_EndFrame();


    SDL_RenderPresent(mRenderer);
}

void UIRenderInterface::setMode(uint16_t width, uint16_t height, const PixelFormat *format, ISDL20Window *window,
                                bool vsync, const char *render_scale_quality)
{
    mWindow = window;

    assert(mWindow != NULL);
    assert(mWindow->mSDLWindow != NULL);

    mWidth  = width;
    mHeight = height;

    memcpy(&mFormat, format, sizeof(mFormat));

    // [jsd] set the user's preferred render scaling hint if non-empty:
    // acceptable values are [("0" or "nearest"), ("1" or "linear"), ("2" or "best")].
    bool quality_set = false;
    if (render_scale_quality != NULL && render_scale_quality[0] != '\0')
    {
        if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, render_scale_quality) == SDL_TRUE)
        {
            quality_set = true;
        }
    }

    if (!quality_set)
    {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    }

    const char *driver = mWindow->getRendererDriver();

    uint32_t renderer_flags = 0;
    if (strncmp(driver, "software", strlen(driver)) == 0)
        renderer_flags |= SDL_RENDERER_SOFTWARE;
    else
        renderer_flags |= SDL_RENDERER_ACCELERATED;
    if (vsync)
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;

    if (!mRenderer)
    {
        mRenderer = SDL_CreateRenderer(mWindow->mSDLWindow, -1, renderer_flags);
    }
    

    if (mRenderer == NULL)
        I_Error("I_InitVideo: unable to create SDL2 renderer: %s\n", SDL_GetError());

    const IVideoMode &native_mode = I_GetVideoCapabilities()->getNativeMode();
    SDL_RenderSetLogicalSize(mRenderer, mWidth, mHeight);

    // Ensure the game window is clear, even if using -noblit
    SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
    SDL_RenderClear(mRenderer);
    SDL_RenderPresent(mRenderer);

    SDL_DisplayMode sdl_mode;
    SDL_GetWindowDisplayMode(mWindow->mSDLWindow, &sdl_mode);
    mSDLDisplayFormat = sdl_mode.format;

    void UI_IMGUI_Init();
    UI_IMGUI_Init();

    // Fixme:  THIS DOES NOT BELONG HERE
    void UI_LoadCore();
    UI_LoadCore();
}

Rml::CompiledGeometryHandle UIRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                               Rml::Span<const int32_t>         indices)
{
    GeometryView *data = new GeometryView{vertices, indices};
    return reinterpret_cast<Rml::CompiledGeometryHandle>(data);
}

void UIRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
    delete reinterpret_cast<GeometryView *>(geometry);
}

void UIRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation,
                                       Rml::TextureHandle texture)
{
    const GeometryView *geometry     = reinterpret_cast<GeometryView *>(handle);
    const Rml::Vertex  *vertices     = geometry->vertices.data();
    const size_t        num_vertices = geometry->vertices.size();
    const int32_t          *indices      = geometry->indices.data();
    const size_t        num_indices  = geometry->indices.size();

    SDL_FPoint *positions = new SDL_FPoint[num_vertices];

    for (size_t i = 0; i < num_vertices; i++)
    {
        positions[i].x = vertices[i].position.x + translation.x;
        positions[i].y = vertices[i].position.y + translation.y;
    }

    SDL_Texture *sdl_texture = (SDL_Texture *)texture;

    SDL_RenderGeometryRaw(mRenderer, sdl_texture, &positions[0].x, sizeof(SDL_FPoint),
                          (const SDL_Color *)&vertices->colour, sizeof(Rml::Vertex), &vertices->tex_coord.x,
                          sizeof(Rml::Vertex), (int32_t)num_vertices, indices, (int32_t)num_indices, 4);

    delete[] positions;
}

void UIRenderInterface::EnableScissorRegion(bool enable)
{
    if (enable)
        SDL_RenderSetClipRect(mRenderer, &mRectScissor);
    else
        SDL_RenderSetClipRect(mRenderer, nullptr);

    mScissorRegionEnabled = enable;
}

void UIRenderInterface::SetScissorRegion(Rml::Rectanglei region)
{
    mRectScissor.x = region.Left();
    mRectScissor.y = region.Top();
    mRectScissor.w = region.Width();
    mRectScissor.h = region.Height();

    if (mScissorRegionEnabled)
        SDL_RenderSetClipRect(mRenderer, &mRectScissor);
}

Rml::TextureHandle UIRenderInterface::LoadTexture(Rml::Vector2i &texture_dimensions, const Rml::String &source)
{
    if (source == "*PLAYER_VIEW")
    {
        if (mPlayerViewTexture)
        {
            return (Rml::TextureHandle)mPlayerViewTexture;
        }

        uint32_t texture_flags = SDL_TEXTUREACCESS_STREAMING;

        texture_dimensions.x = mWidth;
        texture_dimensions.y = mHeight;

        mPlayerViewTexture = SDL_CreateTexture(mRenderer, mSDLDisplayFormat, texture_flags,
                                                                          texture_dimensions.x, texture_dimensions.y);

        return (Rml::TextureHandle)mPlayerViewTexture;
    }

    Rml::FileInterface *file_interface = Rml::GetFileInterface();
    Rml::FileHandle     file_handle    = file_interface->Open(source);
    if (!file_handle)
        return {};

    file_interface->Seek(file_handle, 0, SEEK_END);
    size_t buffer_size = file_interface->Tell(file_handle);
    file_interface->Seek(file_handle, 0, SEEK_SET);

    using Rml::byte;
    Rml::UniquePtr<byte[]> buffer(new byte[buffer_size]);
    file_interface->Read(buffer.get(), buffer_size, file_handle);
    file_interface->Close(file_handle);

    const size_t i_ext     = source.rfind('.');
    Rml::String  extension = (i_ext == Rml::String::npos ? Rml::String() : source.substr(i_ext + 1));

    int32_t      width, height, channels;
    stbi_uc *image_data = stbi_load_from_memory(buffer.get(), int32_t(buffer_size), &width, &height, &channels, 0);

    Uint32 format = (channels == 3) ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGBA32;

    SDL_Surface *surface =
        SDL_CreateRGBSurfaceWithFormatFrom((void *)image_data, width, height, channels * 8, channels * width, format);

    if (!surface)
        return {};

    if (surface->format->format != SDL_PIXELFORMAT_RGBA32 && surface->format->format != SDL_PIXELFORMAT_BGRA32)
    {
        SDL_Surface *converted_surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);

        if (!converted_surface)
            return {};

        surface = converted_surface;
    }

    // Convert colors to premultiplied alpha, which is necessary for correct alpha compositing.
    byte *pixels = static_cast<byte *>(surface->pixels);
    for (int32_t i = 0; i < surface->w * surface->h * 4; i += 4)
    {
        const byte alpha = pixels[i + 3];
        for (int32_t j = 0; j < 3; ++j)
            pixels[i + j] = byte(int32_t(pixels[i + j]) * int32_t(alpha) / 255);
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(mRenderer, surface);

    texture_dimensions = Rml::Vector2i(surface->w, surface->h);
    SDL_FreeSurface(surface);

    if (texture)
        SDL_SetTextureBlendMode(texture, mBlendMode);

    return (Rml::TextureHandle)texture;
}

Rml::TextureHandle UIRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source,
                                                      Rml::Vector2i              source_dimensions)
{
    SDL_Surface *surface =
        SDL_CreateRGBSurfaceWithFormatFrom((void *)source.data(), source_dimensions.x, source_dimensions.y, 32,
                                           source_dimensions.x * 4, SDL_PIXELFORMAT_RGBA32);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(mRenderer, surface);
    SDL_SetTextureBlendMode(texture, mBlendMode);

    SDL_FreeSurface(surface);
    return (Rml::TextureHandle)texture;
}

void UIRenderInterface::ReleaseTexture(Rml::TextureHandle texture_handle)
{
    if (mPlayerViewTexture == (SDL_Texture *)texture_handle)
    {
        mPlayerViewTexture = nullptr;
    }

    SDL_DestroyTexture((SDL_Texture *)texture_handle);
}

void UIRenderInterface::setRenderSurface(IRenderSurface *surface)
{
    if (mRenderSurface)
    {
        // delete mRenderSurface;
    }

    mRenderSurface = surface;
    R_ForceViewWindowResize();
}

void UI_SetMode(uint16_t width, uint16_t height, const PixelFormat *format, ISDL20Window *window, bool vsync,
                const char *render_scale_quality)
{
    UIRenderInterface *renderInterface = (UIRenderInterface *)Rml::GetRenderInterface();
    if (!renderInterface)
    {
        I_Error("No renderer interface");
    }

    renderInterface->setMode(width, height, format, window, vsync, render_scale_quality);
}
