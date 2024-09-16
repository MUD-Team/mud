//------------------------------------------------------------------------
//  Sound Format Detection
//------------------------------------------------------------------------
//
//  Copyright (c) 2022-2023 - The EDGE Team.
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
//------------------------------------------------------------------------

#include "snd_types.h"

#include "epi.h"
#include "epi_filesystem.h"
#include "epi_str_util.h"

SoundFormat DetectSoundFormat(uint8_t *data, int song_len)
{
    // Start by trying the simple reliable header checks

    if (data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F')
    {
        return kSoundWAV;
    }

    if (data[0] == 'f' && data[1] == 'L' && data[2] == 'a' && data[3] == 'C')
    {
        return kSoundFLAC;
    }

    if (data[0] == 'O' && data[1] == 'g' && data[2] == 'g' && data[3] == 'S')
    {
        return kSoundOGG;
    }

    if (data[0] == 'M' && data[1] == 'U' && data[2] == 'S')
    {
        return kSoundMUS;
    }

    if (data[0] == 'M' && data[1] == 'T' && data[2] == 'h' && data[3] == 'd')
    {
        return kSoundMIDI;
    }

    if (data[0] == 0x3)
    {
        return kSoundDoom;
    }

    return kSoundUnknown;
}

SoundFormat SoundFilenameToFormat(std::string_view filename)
{
    std::string ext = epi::GetExtension(filename);

    epi::StringLowerASCII(ext);

    if (ext == ".wav" || ext == ".wave")
        return kSoundWAV;

    if (ext == ".flac")
        return kSoundFLAC;

    if (ext == ".ogg")
        return kSoundOGG;

    if (ext == ".mus")
        return kSoundMUS;

    if (ext == ".mid" || ext == ".midi")
        return kSoundMIDI;

    // Not sure if these will ever be encountered in the wild, but according to
    // the VGMPF Wiki they are valid DMX file extensions
    if (ext == ".dsp" || ext == ".pcs" || ext == ".gsp" || ext == ".gsw" || ext == ".lmp")
        return kSoundDoom;

    return kSoundUnknown;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
