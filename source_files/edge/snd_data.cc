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

#include "HandmadeMath.h"
#include "epi.h"

SoundData::SoundData()
    : length_(0), frequency_(0), mode_(0), data_left_(nullptr), data_right_(nullptr), definition_data_(nullptr)
{
}

SoundData::~SoundData()
{
    Free();
}

void SoundData::Free()
{
    length_ = 0;

    if (data_right_ && data_right_ != data_left_)
        delete[] data_right_;

    if (data_left_)
        delete[] data_left_;

    data_left_  = nullptr;
    data_right_ = nullptr;
}

void SoundData::Allocate(int samples, int buf_mode)
{
    // early out when requirements are already met
    if (data_left_ && length_ >= samples && mode_ == buf_mode)
    {
        length_ = samples; // FIXME: perhaps keep allocated count
        return;
    }

    if (data_left_ || data_right_)
    {
        Free();
    }

    length_ = samples;
    mode_   = buf_mode;

    switch (buf_mode)
    {
    case kMixMono:
        data_left_  = new int16_t[samples];
        data_right_ = data_left_;
        break;

    case kMixStereo:
        data_left_  = new int16_t[samples];
        data_right_ = new int16_t[samples];
        break;

    case kMixInterleaved:
        data_left_  = new int16_t[samples * 2];
        data_right_ = data_left_;
        break;

    default:
        break;
    }
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
