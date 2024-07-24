//----------------------------------------------------------------------------
//  Sound Data
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

#include "snd_data.h"

#include <vector>

#include "epi.h"

SoundData::SoundData()
    : length_(0), frequency_(0), mode_(0), data_(nullptr), definition_data_(nullptr)
{
}

SoundData::~SoundData()
{
    Free();
}

void SoundData::Free()
{
    length_ = 0;

    if (data_)
        delete[] data_;

    data_  = nullptr;
}

void SoundData::Allocate(int samples, int buf_mode)
{
    // early out when requirements are already met
    if (data_ && length_ >= samples && mode_ == buf_mode)
    {
        length_ = samples; // FIXME: perhaps keep allocated count
        return;
    }

    if (data_)
    {
        Free();
    }

    length_ = samples;
    mode_   = buf_mode;

    switch (buf_mode)
    {
    case kMixMono:
        data_  = new float[samples];
        break;

    case kMixInterleaved:
        data_  = new float[samples * 2];
        break;

    default:
        break;
    }
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
