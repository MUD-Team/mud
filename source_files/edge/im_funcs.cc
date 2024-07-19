//------------------------------------------------------------------------
//  Image Handling
//----------------------------------------------------------------------------
//
//  Copyright (c) 2003-2024 The EDGE Team.
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

#include "im_funcs.h"

#include "epi.h"
#include "epi_endian.h"
#include "epi_filesystem.h"
#include "epi_str_util.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

ImageAtlas::ImageAtlas(int w, int h)
{
    data_ = new ImageData(w, h, 4);
    memset(data_->pixels_, 0, w * h * 4);
}

ImageAtlas::~ImageAtlas()
{
    delete data_;
    data_ = nullptr;
}

ImageFormat DetectImageFormat(uint8_t *header, int header_length, int file_size)
{
    if (header_length < 12)
        return kImageUnknown;

    // PNG is clearly marked in the header, so check it first.

    if (header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G' && header[4] == 0x0D &&
        header[5] == 0x0A)
    {
        return kImagePNG;
    }

    return kImageUnknown; // uh oh!
}

ImageFormat ImageFormatFromFilename(const std::string &filename)
{
    std::string ext = epi::GetExtension(filename);

    epi::StringLowerASCII(ext);

    if (ext == ".png")
        return kImagePNG;

    return kImageUnknown;
}

ImageData *LoadImageData(epi::File *file)
{
    int width  = 0;
    int height = 0;
    int depth = 0;

    int      length    = file->GetLength();
    uint8_t *raw_image = file->LoadIntoMemory();

    uint8_t *decoded_img = stbi_load_from_memory(raw_image, length, &width, &height, &depth, 0);

    // we don't want no grayscale here, force STB to convert
    if (decoded_img != nullptr && (depth == 1 || depth == 2))
    {
        stbi_image_free(decoded_img);

        // depth_ 1 = grayscale, so force RGB
        // depth_ 2 = grayscale + alpha, so force RGBA
        int new_depth = depth + 2;

        decoded_img = stbi_load_from_memory(raw_image, length, &width, &height, &depth, new_depth);

        depth = new_depth; // sigh...
    }

    delete[] raw_image;

    if (decoded_img == nullptr)
        return nullptr;

    int total_w = width;
    int total_h = height;

    // round size up to the nearest power-of-two
    if (true)
    {
        total_w = 1;
        while (total_w < (int)width)
            total_w <<= 1;
        total_h = 1;
        while (total_h < (int)height)
            total_h <<= 1;
    }

    ImageData *img = new ImageData(total_w, total_h, depth);

    img->used_width_  = width;
    img->used_height_ = height;

    if (img->used_width_ != total_w || img->used_height_ != total_h)
        img->Clear();

    // copy the image data, inverting it at the same time
    for (int y = 0; y < height; y++)
    {
        const uint8_t *source = &decoded_img[(height - 1 - y) * width * depth];
        memcpy(img->PixelAt(0, y), source, width * depth);
    }

    stbi_image_free(decoded_img);

    return img;
}

ImageAtlas *PackImages(const std::unordered_map<int, ImageData *> &image_pack_data)
{
    stbrp_node              nodes[4096]; // Max OpenGL texture width we allow
    std::vector<stbrp_rect> rects;
    // These should only grow up to the minimum coverage, which is hopefully
    // less than 4096 since stb_rect_pack indicates the number of nodes should
    // be higher than the actual width for best results
    int atlas_w = 1;
    int atlas_h = 1;
    for (std::pair<const int, ImageData *> im : image_pack_data)
    {
        EPI_ASSERT(im.second->depth_ >= 3);
        if (im.second->depth_ == 3)
            im.second->SetAlpha(255);
        stbrp_rect rect;
        rect.id = im.first;
        rect.w  = im.second->used_width_ + 2;
        rect.h  = im.second->used_height_ + 2;
        if (rect.w > atlas_w)
        {
            atlas_w = 1;
            while (atlas_w < (int)rect.w)
                atlas_w <<= 1;
        }
        if (rect.h > atlas_h)
        {
            atlas_h = 1;
            while (atlas_h < (int)rect.h)
                atlas_h <<= 1;
        }
        rects.push_back(rect);
    }
    if (atlas_h < atlas_w)
        atlas_h = atlas_w;
    stbrp_context ctx;
    stbrp_init_target(&ctx, atlas_w, atlas_h, nodes, 4096);
    int packres = stbrp_pack_rects(&ctx, rects.data(), rects.size());
    while (packres != 1)
    {
        atlas_w *= 2;
        if (atlas_h < atlas_w)
            atlas_h = atlas_w;
        if (atlas_w > 4096 || atlas_h > 4096)
            FatalError("PackImages: Atlas exceeds maximum allowed texture size "
                       "(4096x4096)!");
        stbrp_init_target(&ctx, atlas_w, atlas_h, nodes, 4096);
        packres = stbrp_pack_rects(&ctx, rects.data(), rects.size());
    }
    ImageAtlas *atlas = new ImageAtlas(atlas_w, atlas_h);
    // fill atlas image_data_c
    for (size_t i = 0; i < rects.size(); i++)
    {
        int        rect_x = rects[i].x + 1;
        int        rect_y = rects[i].y + 1;
        ImageData *im     = image_pack_data.at(rects[i].id);
        for (short x = 0; x < im->used_width_; x++)
        {
            for (short y = 0; y < im->used_height_; y++)
            {
                memcpy(atlas->data_->PixelAt(rect_x + x, rect_y + y), im->PixelAt(x, y), 4);
            }
        }
        ImageAtlasRectangle atlas_rect;
        atlas_rect.texture_coordinate_x      = (float)rect_x / atlas_w;
        atlas_rect.texture_coordinate_y      = (float)rect_y / atlas_h;
        atlas_rect.texture_coordinate_width  = (float)im->used_width_ / atlas_w;
        atlas_rect.texture_coordinate_height = (float)im->used_height_ / atlas_h;
        atlas_rect.image_width               = im->used_width_ * im->scale_x_;
        atlas_rect.image_height              = im->used_height_ * im->scale_y_;
        atlas_rect.offset_x                  = im->offset_x_;
        atlas_rect.offset_y                  = im->offset_y_;
        atlas->rectangles_.try_emplace(rects[i].id, atlas_rect);
    }
    return atlas;
}

bool GetImageInfo(epi::File *file, int *width, int *height, int *depth)
{
    int      length    = file->GetLength();
    uint8_t *raw_image = file->LoadIntoMemory();

    int result = stbi_info_from_memory(raw_image, length, width, height, depth);

    delete[] raw_image;

    return result != 0;
}

//------------------------------------------------------------------------

static void STBImageEPIFileWrite(void *context, void *data, int size)
{
    EPI_ASSERT(context && data && size);
    epi::File *dest = (epi::File *)context;
    dest->Write(data, size);
}

bool SavePNG(const std::string &filename, ImageData *image)
{
    EPI_ASSERT(image->depth_ >= 3);

    epi::File *dest = epi::FileOpen(filename, epi::kFileAccessWrite);

    if (!dest)
        return false;

    // zero means failure here
    int result = stbi_write_png_to_func(STBImageEPIFileWrite, dest, image->used_width_, image->used_height_,
                                        image->depth_, image->pixels_, 0);

    delete dest;

    if (result == 0)
    {
        epi::FileDelete(filename);
        return false;
    }
    else
        return true;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
