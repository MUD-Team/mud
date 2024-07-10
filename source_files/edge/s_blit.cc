//----------------------------------------------------------------------------
//  Sound Blitter
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

#include "s_blit.h"

#include <list>

#include "dm_state.h"
#include "epi.h"
#include "epi_sdl.h"
#include "i_system.h"
#include "m_misc.h"
#include "p_blockmap.h"
#include "p_local.h" // ApproximateDistance
#include "r_misc.h"  // PointToAngle
#include "s_cache.h"
#include "s_music.h"
#include "s_sound.h"

// Sound must be clipped to prevent distortion (clipping is
// a kind of distortion of course, but it's much better than
// the "white noise" you get when values overflow).
//
static constexpr float kSoundClipMaximum =  1.0f;
static constexpr float kSoundClipMinimum = -1.0f;

static constexpr uint8_t  kMinimumSoundChannels = 32;
static constexpr uint16_t kMaximumSoundChannels = 256;

SoundChannel *mix_channels[kMaximumSoundChannels];
int           total_channels;

float music_player_gain       = 1.0f;

static float *mix_buffer;
static int    mix_buffer_length;

static constexpr uint8_t kMaximumQueueBuffers = 16;

static std::list<SoundData *> free_queue_buffers;
static std::list<SoundData *> playing_queue_buffers;

static SoundChannel *queue_channel;

EDGE_DEFINE_CONSOLE_VARIABLE(sound_effect_volume, "0.15", kConsoleVariableFlagArchive)

static bool sound_effects_paused = false;

// these are analogous to view_x/y/z/angle
float    listen_x;
float    listen_y;
float    listen_z;
BAMAngle listen_angle;

extern int sound_device_frequency;
extern int sound_device_bytes_per_sample;
extern int sound_device_samples_per_buffer;

extern bool sound_device_stereo;

SoundChannel::SoundChannel() : state_(kChannelEmpty), data_(nullptr)
{
}

SoundChannel::~SoundChannel()
{
}

void SoundChannel::ComputeDelta()
{
    delta_ = (float)data_->frequency_ * (1.0f / (float)sound_device_frequency);
}

void SoundChannel::ComputeVolume()
{
    float sep  = 0.5f;
    float dist = 1.25f;

    if (position_ && category_ >= kCategoryOpponent)
    {
        if (sound_device_stereo)
        {
            BAMAngle angle = PointToAngle(listen_x, listen_y, position_->x, position_->y);

            // same equation from original DOOM
            sep = 0.5f - 0.38f * epi::BAMSin(angle - listen_angle);
        }

        if (!boss_)
        {
            dist = ApproximateDistance(listen_x - position_->x, listen_y - position_->y, listen_z - position_->z);
            dist = HMM_MAX(1.25f, dist / 100.0f);
        }
    }

    float MAX_VOL = 1.0f;

    MAX_VOL = (boss_ ? MAX_VOL : MAX_VOL / dist) * sound_effect_volume.f_;

    if (definition_)
        MAX_VOL *= definition_->volume_;

    // strictly linear equations
    volume_left_  = (MAX_VOL * (1.0 - sep));
    volume_right_ = (MAX_VOL * (0.0 + sep));

    if (var_sound_stereo == 2) /* SWAP ! */
    {
        float tmp       = volume_left_;
        volume_left_    = volume_right_;
        volume_right_   = tmp;
    }
}

void SoundChannel::ComputeMusicVolume()
{
    float MAX_VOL = 1.0f;

    MAX_VOL = MAX_VOL * music_volume.f_ * music_player_gain; // This last one is an internal value that depends on music format

    volume_left_  = MAX_VOL;
    volume_right_ = MAX_VOL;
}

//----------------------------------------------------------------------------

static void BlitToF32(const float *src, float *dest, int length)
{
    const float *s_end = src + length;

    while (src < s_end)
    {
        float val = *src++;
        
        HMM_Clamp(kSoundClipMinimum, val, kSoundClipMaximum);

        *dest++ = val;
    }
}

static void MixMono(SoundChannel *chan, float *dest, int pairs)
{
    EPI_ASSERT(pairs > 0);

    float *src = chan->data_->data_;

    float *d_pos = dest;
    float *d_end = d_pos + pairs;

    float offset = chan->offset_;

    while (d_pos < d_end)
    {
        *d_pos++ += src[(uint32_t)offset] * chan->volume_left_;

        offset += chan->delta_;
    }

    chan->offset_ = offset;

    EPI_ASSERT((uint32_t)(offset - chan->delta_) < chan->length_);
}

static void MixInterleaved(SoundChannel *chan, float *dest, int pairs)
{
    if (!sound_device_stereo)
        FatalError("INTERNAL ERROR: tried to mix an interleaved buffer in MONO "
                   "mode.\n");

    EPI_ASSERT(pairs > 0);

    float *src = chan->data_->data_;

    float *d_pos = dest;
    float *d_end = d_pos + pairs * 2;

    float offset = chan->offset_;

    while (d_pos < d_end)
    {
        uint32_t pos = (uint32_t)(offset * 2.0f) & ~1;

        *d_pos++ += src[pos] * chan->volume_left_;
        *d_pos++ += src[pos|1] * chan->volume_right_;
        offset += chan->delta_;
    }

    chan->offset_ = offset;

    EPI_ASSERT((uint32_t)(offset - chan->delta_) < chan->length_);
}

static void MixOneChannel(SoundChannel *chan, int pairs)
{
    if (sound_effects_paused && chan->category_ >= kCategoryPlayer)
        return;

    if (AlmostEquals(chan->volume_left_, 0.0f) && AlmostEquals(chan->volume_right_, 0.0f))
        return;

    EPI_ASSERT((uint32_t)chan->offset_ < chan->length_);

    float *dest = mix_buffer;

    while (pairs > 0)
    {
        int count = pairs;

        // check if enough sound data is left
        if ((uint32_t)(chan->offset_ + count * chan->delta_) >= chan->length_)
        {
            // find minimum number of samples we can play
            double avail = (chan->length_ - (uint32_t)(chan->offset_)) / (double)chan->delta_;

            count = (int)floor(avail);

            EPI_ASSERT(count > 0);
            EPI_ASSERT(count <= pairs);

            EPI_ASSERT((uint32_t)RoundToInteger(chan->offset_ + count * chan->delta_) >= chan->length_);
        }

        if (chan->data_->mode_ == kMixInterleaved)
            MixInterleaved(chan, dest, count);
        else
            MixMono(chan, dest, count);

        if ((uint32_t)chan->offset_ >= chan->length_)
        {
            if (!chan->loop_)
            {
                chan->state_ = kChannelFinished;
                break;
            }

            // we are looping, so clear flag.  The sound needs to
            // be "pumped" (played again) to continue looping.
            chan->loop_ = false;

            chan->offset_ = 0;
        }

        dest += count * (sound_device_stereo ? 2 : 1);
        pairs -= count;
    }
}

static bool QueueNextBuffer(void)
{
    if (playing_queue_buffers.empty())
    {
        queue_channel->state_ = kChannelFinished;
        queue_channel->data_  = nullptr;
        return false;
    }

    SoundData *buf = playing_queue_buffers.front();

    queue_channel->data_ = buf;

    queue_channel->offset_ = 0;
    queue_channel->length_ = buf->length_;

    queue_channel->ComputeDelta();

    queue_channel->state_ = kChannelPlaying;
    return true;
}

static void MixQueues(int pairs)
{
    SoundChannel *chan = queue_channel;

    if (!chan || !chan->data_ || chan->state_ != kChannelPlaying)
        return;

    if (chan->volume_left_ == 0 && chan->volume_right_ == 0)
        return;

    EPI_ASSERT((uint32_t)chan->offset_ < chan->length_);

    float *dest = mix_buffer;

    while (pairs > 0)
    {
        int count = pairs;

        // check if enough sound data is left
        if ((uint32_t)(chan->offset_ + count * chan->delta_) >= chan->length_)
        {
            // find minimum number of samples we can play
            double avail = (chan->length_ - (uint32_t)(chan->offset_)) / (double)chan->delta_;

            count = (int)floor(avail);

            EPI_ASSERT(count > 0);
            EPI_ASSERT(count <= pairs);

            EPI_ASSERT((uint32_t)RoundToInteger(chan->offset_ + count * chan->delta_) >= chan->length_);
        }

        if (chan->data_->mode_ == kMixInterleaved)
            MixInterleaved(chan, dest, count);
        else
            MixMono(chan, dest, count);

        if ((uint32_t)chan->offset_ >= chan->length_)
        {
            // reached end of current queued buffer.
            // Place current buffer onto free list,
            // and enqueue the next buffer to play.

            EPI_ASSERT(!playing_queue_buffers.empty());

            SoundData *buf = playing_queue_buffers.front();
            playing_queue_buffers.pop_front();

            free_queue_buffers.push_back(buf);

            if (!QueueNextBuffer())
                break;
        }

        dest += count * (sound_device_stereo ? 2 : 1);
        pairs -= count;
    }
}

void MixAllSoundChannels(void *stream, int len)
{
    if (no_sound || len <= 0)
        return;

    int pairs = len;

    int samples = pairs;
    if (sound_device_stereo)
        samples *= 2;

    // check that we're not getting too much data
    EPI_ASSERT(pairs <= sound_device_samples_per_buffer);

    EPI_ASSERT(mix_buffer && samples <= mix_buffer_length);

    // clear mixer buffer
    memset(mix_buffer, 0, mix_buffer_length * sizeof(float));

    // add each channel
    for (int i = 0; i < total_channels; i++)
    {
        if (mix_channels[i]->state_ == kChannelPlaying)
        {
            MixOneChannel(mix_channels[i], pairs);
        }
    }

    MixQueues(pairs);

    // blit to the SDL stream
    BlitToF32(mix_buffer, (float *)stream, samples);
}

//----------------------------------------------------------------------------

void InitializeSoundChannels(int total)
{
    // NOTE: assumes audio is locked!

    EPI_ASSERT(total >= kMinimumSoundChannels);
    EPI_ASSERT(total <= kMaximumSoundChannels);

    total_channels = total;

    for (int i = 0; i < total_channels; i++)
        mix_channels[i] = new SoundChannel();

    // allocate mixer buffer
    mix_buffer_length = sound_device_samples_per_buffer * (sound_device_stereo ? 2 : 1);
    mix_buffer        = new float[mix_buffer_length];
}

void FreeSoundChannels(void)
{
    // NOTE: assumes audio is locked!

    for (int i = 0; i < total_channels; i++)
    {
        SoundChannel *chan = mix_channels[i];

        if (chan && chan->data_)
        {
            chan->data_ = nullptr;
        }

        delete chan;
    }

    memset(mix_channels, 0, sizeof(mix_channels));
}

void KillSoundChannel(int k)
{
    SoundChannel *chan = mix_channels[k];

    if (chan->state_ != kChannelEmpty)
    {
        chan->data_  = nullptr;
        chan->state_ = kChannelEmpty;
    }
}

void ReallocateSoundChannels(int total)
{
    // NOTE: assumes audio is locked!

    EPI_ASSERT(total >= kMinimumSoundChannels);
    EPI_ASSERT(total <= kMaximumSoundChannels);

    if (total > total_channels)
    {
        for (int i = total_channels; i < total; i++)
            mix_channels[i] = new SoundChannel();
    }

    if (total < total_channels)
    {
        // kill all non-UI sounds, pack the UI sounds into the
        // remaining slots (normally there will be enough), and
        // delete the unused channels
        int i, j;

        for (i = 0; i < total_channels; i++)
        {
            SoundChannel *chan = mix_channels[i];

            if (chan->state_ == kChannelPlaying)
            {
                if (chan->category_ != kCategoryUi)
                    KillSoundChannel(i);
            }
        }

        for (i = j = 0; i < total_channels; i++)
        {
            if (mix_channels[i])
            {
                /* SWAP ! */
                SoundChannel *tmp = mix_channels[j];

                mix_channels[j] = mix_channels[i];
                mix_channels[i] = tmp;
            }
        }

        for (i = total; i < total_channels; i++)
        {
            if (mix_channels[i]->state_ == kChannelPlaying)
                KillSoundChannel(i);

            delete mix_channels[i];
            mix_channels[i] = nullptr;
        }
    }

    total_channels = total;
}

void UpdateSounds(Position *listener, BAMAngle angle)
{
    listen_x = listener ? listener->x : 0;
    listen_y = listener ? listener->y : 0;
    listen_z = listener ? listener->z : 0;

    listen_angle = angle;

    for (int i = 0; i < total_channels; i++)
    {
        SoundChannel *chan = mix_channels[i];

        if (chan->state_ == kChannelPlaying)
            chan->ComputeVolume();

        else if (chan->state_ == kChannelFinished)
            KillSoundChannel(i);
    }

    if (queue_channel)
        queue_channel->ComputeMusicVolume();
}

void PauseSound(void)
{
    sound_effects_paused = true;
}

void ResumeSound(void)
{
    sound_effects_paused = false;
}

//----------------------------------------------------------------------------

void SoundQueueInitialize(void)
{
    if (no_sound)
        return;

    if (free_queue_buffers.empty())
    {
        for (int i = 0; i < kMaximumQueueBuffers; i++)
        {
            free_queue_buffers.push_back(new SoundData());
        }
    }

    if (!queue_channel)
        queue_channel = new SoundChannel();

    queue_channel->state_ = kChannelEmpty;
    queue_channel->data_  = nullptr;

    queue_channel->ComputeMusicVolume();
}

void SoundQueueShutdown(void)
{
    if (no_sound)
        return;

    if (queue_channel)
    {
        // free all data on the playing / free lists.
        // The SoundData destructor takes care of data_left_/R.

        for (; !playing_queue_buffers.empty(); playing_queue_buffers.pop_front())
        {
            delete playing_queue_buffers.front();
        }
        for (; !free_queue_buffers.empty(); free_queue_buffers.pop_front())
        {
            delete free_queue_buffers.front();
        }

        queue_channel->data_ = nullptr;

        delete queue_channel;
        queue_channel = nullptr;
    }
}

void SoundQueueStop(void)
{
    if (no_sound)
        return;

    EPI_ASSERT(queue_channel);

    for (; !playing_queue_buffers.empty(); playing_queue_buffers.pop_front())
    {
        free_queue_buffers.push_back(playing_queue_buffers.front());
    }

    queue_channel->state_ = kChannelFinished;
    queue_channel->data_  = nullptr;
}

SoundData *SoundQueueGetFreeBuffer(int samples, int buf_mode)
{
    if (no_sound)
        return nullptr;

    SoundData *buf = nullptr;

    if (!free_queue_buffers.empty())
    {
        buf = free_queue_buffers.front();
        free_queue_buffers.pop_front();

        buf->Allocate(samples, buf_mode);
    }

    return buf;
}

void SoundQueueAddBuffer(SoundData *buf, int freq)
{
    EPI_ASSERT(!no_sound);
    EPI_ASSERT(buf);

    buf->frequency_ = freq;

    playing_queue_buffers.push_back(buf);

    if (queue_channel->state_ != kChannelPlaying)
    {
        QueueNextBuffer();
    }
}

void SoundQueueReturnBuffer(SoundData *buf)
{
    EPI_ASSERT(!no_sound);
    EPI_ASSERT(buf);

    free_queue_buffers.push_back(buf);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
