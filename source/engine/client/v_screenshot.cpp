// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 2f24cb1846fd19e022aabff7cfd82d341c44a869 $
//
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//
// Screenshots
//
//-----------------------------------------------------------------------------

#include <SDL.h>
#include <stdlib.h>

#include "c_dispatch.h"
#include "cmdlib.h"
#include "g_game.h"
#include "i_system.h"
#include "i_video.h"
#include "m_fileio.h"
#include "m_misc.h"
#include "odamex.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "v_palette.h"


bool M_FindFreeName(std::string &filename, const std::string &extension);

EXTERN_CVAR(gammalevel)
EXTERN_CVAR(vid_gammatype)

CVAR_FUNC_IMPL(cl_screenshotname)
{
    // No empty format strings allowed.
    if (strlen(var.cstring()) == 0)
        var.RestoreDefault();
}

BEGIN_COMMAND(screenshot)
{
    if (argc == 1)
        G_ScreenShot(NULL);
    else
        G_ScreenShot(argv[1]);
}
END_COMMAND(screenshot)

//
// V_SavePNG
//
// Converts the surface to PNG format and saves it to filename.
// Supporting function for I_ScreenShot to output PNG files.
//
static bool V_SavePNG(const std::string &filename, IWindowSurface *surface)
{
    surface->lock();
    int width  = surface->getWidth();
    int height = surface->getHeight();

    // allocate memory space for PNG pixel data
    uint8_t *pixels = new uint8_t[width * height * 3];

    // convert paletted image to RGB
    if (surface->getBitsPerPixel() == 8)
    {
        const palette_t  *current_palette = V_GetDefaultPalette();
        const palindex_t *source          = (palindex_t *)surface->getBuffer();
        const int         pitch_remainder = surface->getPitchInPixels() - width;

        for (unsigned int y = 0; y < height; y++)
        {
            uint8_t *pixel = pixels+(y*width*3);

            for (unsigned int x = 0; x < width; x++)
            {               
                const argb_t &color = current_palette->colors[*source++];
                // write color components to current pixel of PNG row
                // note: PNG is a big-endian file format
                *pixel++ = (uint8_t)color.getr();
                *pixel++ = (uint8_t)color.getg();
                *pixel++ = (uint8_t)color.getb();
            }

            source += pitch_remainder;
        }
    }
    else
    {
        const argb_t *source          = (argb_t *)surface->getBuffer();
        const int     pitch_remainder = surface->getPitchInPixels() - width;

        for (unsigned int y = 0; y < height; y++)
        {
            uint8_t *pixel = pixels+(y*width*3);

            for (unsigned int x = 0; x < width; x++)
            {
                // gather color components from current pixel of SDL surface
                // note: SDL surface's alpha channel is ignored if present
                const argb_t &color = *source++;

                // write color components to current pixel of PNG row
                // note: PNG is a big-endian file format
                *pixel++ = (uint8_t)color.getr();
                *pixel++ = (uint8_t)color.getg();
                *pixel++ = (uint8_t)color.getb();
            }

            source += pitch_remainder;
        }
    }
    surface->unlock();

    int result = stbi_write_png(filename.c_str(), width, height, 3, pixels, 0);

    delete pixels;

    return result != 0;
}

//
// V_ScreenShot
//
// Dumps the contents of the screen framebuffer to a file. The default output
// format is PNG (if libpng is found at compile-time) with BMP as the fallback.
//
void V_ScreenShot(std::string filename)
{
    const std::string extension("png");

    // If no filename was passed, use the screenshot format variable.
    if (filename.empty())
        filename = cl_screenshotname.str();

    // Expand tokens
    filename = M_ExpandTokens(filename);

    // Turn filename into complete path.
    std::string pathname = M_GetUserFileName(filename);

    // If the file already exists, append numbers.
    if (!M_FindFreeName(pathname, extension))
    {
        Printf(PRINT_WARNING, "I_ScreenShot: Delete some screenshots\n");
        return;
    }

    // Create an SDL_Surface object from our screen buffer
    IWindowSurface *primary_surface = I_GetPrimarySurface();

    if (!V_SavePNG(pathname, primary_surface))
    {
        Printf(PRINT_WARNING, "I_SavePNG:Error while saving %s.%s\n", filename.c_str(), extension.c_str());
        return;
    }

    Printf(PRINT_HIGH, "Screenshot taken: %s.%s\n", filename.c_str(), extension.c_str());
}

VERSION_CONTROL(v_screenshot_cpp, "$Id: 2f24cb1846fd19e022aabff7cfd82d341c44a869 $")
