// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: e4105f5c226a8c1a8d298cf3ee910a59ec303b23 $
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
//
// DESCRIPTION:
//	System interface, sound.
//	[Odamex] Fitted to work with SDL
//
//-----------------------------------------------------------------------------

#include "i_sound.h"

#include <SDL2/SDL_mixer.h>
#include <stdlib.h>

#include "Poco/Buffer.h"
#include "i_music.h"
#include "i_sdl.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_misc.h"
#include "mud_includes.h"
#include "w_wad.h"
#include "z_zone.h"

#define NUM_CHANNELS 32

static int32_t mixer_freq;
static Uint16  mixer_format;
static int32_t mixer_channels;

static bool    sound_initialized = false;
static bool    channel_in_use[NUM_CHANNELS];
static int32_t nextchannel = 0;

EXTERN_CVAR(snd_sfxvolume)
EXTERN_CVAR(snd_musicvolume)
EXTERN_CVAR(snd_crossover)

CVAR_FUNC_IMPL(snd_samplerate)
{
    S_Stop();
    S_Init(snd_sfxvolume, snd_musicvolume);
}

// [Russell] - Chocolate Doom's sound converter code, how awesome!
static bool ConvertibleRatio(int32_t freq1, int32_t freq2)
{
    int32_t ratio;

    if (freq1 > freq2)
    {
        return ConvertibleRatio(freq2, freq1);
    }
    else if ((freq2 % freq1) != 0)
    {
        // Not in a direct ratio

        return false;
    }
    else
    {
        // Check the ratio is a power of 2

        ratio = freq2 / freq1;

        while ((ratio & 1) == 0)
        {
            ratio = ratio >> 1;
        }

        return ratio == 1;
    }
}

// Generic sound expansion function for any sample rate

static void ExpandSoundData(uint8_t *data, int32_t samplerate, int32_t bits, int32_t length, Mix_Chunk *destination)
{
    Sint16 *expanded    = reinterpret_cast<Sint16 *>(destination->abuf);
    size_t  samplecount = length / (bits / 8);

    // Generic expansion if conversion does not work:
    //
    // SDL's audio conversion only works for rate conversions that are
    // powers of 2; if the two formats are not in a direct power of 2
    // ratio, do this naive conversion instead.

    // number of samples in the converted sound

    size_t expanded_length = (static_cast<uint64_t>(samplecount) * mixer_freq) / samplerate;
    size_t expand_ratio    = (samplecount << 8) / expanded_length;

    for (size_t i = 0; i < expanded_length; ++i)
    {
        Sint16  sample;
        int32_t src;

        src = (i * expand_ratio) >> 8;

        // [crispy] Handle 16 bit audio data
        if (bits == 16)
        {
            sample = data[src * 2] | (data[src * 2 + 1] << 8);
        }
        else
        {
            sample = data[src] | (data[src] << 8);
            sample -= 32768;
        }

        // expand mono->stereo

        expanded[i * 2] = expanded[i * 2 + 1] = sample;
    }

    // Perform a low-pass filter on the upscaled sound to filter
    // out high-frequency noise from the conversion process.

    // Low-pass filter for cutoff frequency f:
    //
    // For sampling rate r, dt = 1 / r
    // rc = 1 / 2*pi*f
    // alpha = dt / (rc + dt)

    // Filter to the half sample rate of the original sound effect
    // (maximum frequency, by nyquist)

    float dt    = 1.0f / mixer_freq;
    float rc    = 1.0f / (static_cast<float>(PI) * samplerate);
    float alpha = dt / (rc + dt);

    // Both channels are processed in parallel, hence [i-2]:

    for (size_t i = 2; i < expanded_length * 2; ++i)
    {
        expanded[i] = (Sint16)(alpha * expanded[i] + (1 - alpha) * expanded[i - 2]);
    }
}

static Uint8 *perform_sdlmix_conv(Uint8 *data, Uint32 size, Uint32 *newsize)
{
    Mix_Chunk *chunk;
    SDL_RWops *mem_op;
    Uint8     *ret_data;

    // load, allocate and convert the format from memory
    mem_op = SDL_RWFromMem(data, size);

    if (!mem_op)
    {
        Printf(PRINT_HIGH, "perform_sdlmix_conv - SDL_RWFromMem: %s\n", SDL_GetError());

        return NULL;
    }

    chunk = Mix_LoadWAV_RW(mem_op, 1);

    if (!chunk)
    {
        Printf(PRINT_HIGH, "perform_sdlmix_conv - Mix_LoadWAV_RW: %s\n", Mix_GetError());

        return NULL;
    }

    // return the size
    *newsize = chunk->alen;

    // allocate some space in the zone heap
    ret_data = (Uint8 *)Z_Malloc(chunk->alen, PU_STATIC, NULL);

    // copy the converted data to the return buffer
    memcpy(ret_data, chunk->abuf, chunk->alen);

    // clean up
    Mix_FreeChunk(chunk);
    chunk = NULL;

    return ret_data;
}

static void getsfx(sfxinfo_struct *sfx)
{
    Uint32     new_size = 0;
    Mix_Chunk *chunk;

    if (sfx->filename == NULL)
        return;

    // Just deal with uppercase exported lump names with .lmp extension while testing - Dasho
    std::string sfxfile = StdStringToUpper(sfx->filename);
    sfxfile             = StrFormat("sounds/%s.lmp", sfxfile.c_str());

    if (!M_FileExists(sfxfile))
        return;

    PHYSFS_File *sfxraw = PHYSFS_openRead(sfxfile.c_str());

    if (!sfxraw)
        return;

    uint32_t sfxlength = PHYSFS_fileLength(sfxraw);

    sfx->length = sfxlength;

    Poco::Buffer<uint8_t> data(sfxlength);

    if (PHYSFS_readBytes(sfxraw, data.begin(), sfxlength) != sfxlength)
    {
        PHYSFS_close(sfxraw);
        return;
    }

    // [Russell] is it not a doom sound lump?
    if (((data[1] << 8) | data[0]) != 3)
    {
        chunk            = (Mix_Chunk *)Z_Malloc(sizeof(Mix_Chunk), PU_STATIC, NULL);
        chunk->allocated = 1;
        if (sfxlength < 8) // too short to be anything of interest
        {
            // Let's hope SDL_Mixer checks alen before dereferencing abuf!
            chunk->abuf = NULL;
            chunk->alen = 0;
        }
        else
        {
            chunk->abuf = perform_sdlmix_conv(data.begin(), sfxlength, &new_size);
            chunk->alen = new_size;
        }
        chunk->volume = MIX_MAX_VOLUME;

        sfx->data = chunk;

        PHYSFS_close(sfxraw);

        return;
    }

    const Uint32 samplerate = (data[3] << 8) | data[2];
    Uint32       length     = (data[5] << 8) | data[4];

    // [Russell] - Ignore doom's sound format length info
    // if the lump is longer than the value, fixes exec.wad's ssg
    length = (sfx->length - 8 > length) ? sfx->length - 8 : length;

    Uint32 expanded_length = (uint32_t)((((uint64_t)length) * mixer_freq) / samplerate);

    // Double up twice: 8 -> 16 bit and mono -> stereo

    expanded_length *= 4;

    chunk            = (Mix_Chunk *)Z_Malloc(sizeof(Mix_Chunk), PU_STATIC, NULL);
    chunk->allocated = 1;
    chunk->alen      = expanded_length;
    chunk->abuf      = (Uint8 *)Z_Malloc(expanded_length, PU_STATIC, NULL);
    chunk->volume    = MIX_MAX_VOLUME;

    ExpandSoundData((uint8_t *)data.begin() + 8, samplerate, 8, length, chunk);
    sfx->data = chunk;

    PHYSFS_close(sfxraw);
}

//
// SFX API
//
void I_SetChannels(int32_t numchannels)
{
}

static float basevolume;

void I_SetSfxVolume(float volume)
{
    basevolume = volume;
}

//
// I_StartSound
//
// Starting a sound means adding it to the current list of active sounds
// in the internal channels. As the SFX info struct contains  e.g. a pointer
// to the raw data, it is ignored. As our sound handling does not handle
// priority, it is ignored. Pitching (that is, increased speed of playback)
// is set, but currently not used by mixing.
//
int32_t I_StartSound(int32_t id, float vol, int32_t sep, int32_t pitch, bool loop)
{
    if (!sound_initialized)
        return -1;

    Mix_Chunk *chunk = (Mix_Chunk *)S_sfx[id].data;

    // find a free channel, starting from the first after
    // the last channel we used
    int32_t channel = nextchannel;

    do
    {
        channel = (channel + 1) % NUM_CHANNELS;

        if (channel == nextchannel)
        {
            fprintf(stderr, "No free sound channels left.\n");
            return -1;
        }
    } while (channel_in_use[channel]);

    nextchannel = channel;

    // play sound
    Mix_PlayChannelTimed(channel, chunk, loop ? -1 : 0, -1);

    channel_in_use[channel] = true;

    // set seperation, etc.
    I_UpdateSoundParams(channel, vol, sep, pitch);

    return channel;
}

void I_StopSound(int32_t handle)
{
    if (!sound_initialized)
        return;

    channel_in_use[handle] = false;

    Mix_HaltChannel(handle);
}

int32_t I_SoundIsPlaying(int32_t handle)
{
    if (!sound_initialized)
        return 0;

    return Mix_Playing(handle);
}

void I_UpdateSoundParams(int32_t handle, float vol, int32_t sep, int32_t pitch)
{
    if (!sound_initialized)
        return;

    if (sep > 255)
        sep = 255;

    if (!snd_crossover)
        sep = 255 - sep;

    int32_t volume = (int32_t)((float)MIX_MAX_VOLUME * basevolume * vol);

    if (volume < 0)
        volume = 0;
    if (volume > MIX_MAX_VOLUME)
        volume = MIX_MAX_VOLUME;

    Mix_Volume(handle, volume);
    Mix_SetPanning(handle, sep, 255 - sep);
}

void I_LoadSound(sfxinfo_struct *sfx)
{
    if (!sound_initialized)
        return;

    if (!sfx->data)
    {
        DPrintf("loading sound \"%s\" (%s)\n", sfx->name, sfx->filename);
        getsfx(sfx);
    }
}

void I_InitSound()
{
    if (I_IsHeadless() || Args.CheckParm("-nosound"))
        return;

#if defined(SDL20)
    Printf("I_InitSound: Initializing SDL's sound subsystem\n");
#endif

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        Printf(PRINT_ERROR, "I_InitSound: Unable to set up sound: %s\n", SDL_GetError());

        return;
    }

#if defined(SDL20)
    Printf("I_InitSound: Using SDL's audio driver (%s)\n", SDL_GetCurrentAudioDriver());
#endif

    const SDL_version *ver = Mix_Linked_Version();

    if (ver->major != MIX_MAJOR_VERSION || ver->minor != MIX_MINOR_VERSION)
    {
        Printf(PRINT_ERROR, "I_InitSound: SDL_mixer version conflict (%d.%d.%d vs %d.%d.%d dll)\n", MIX_MAJOR_VERSION,
               MIX_MINOR_VERSION, MIX_PATCHLEVEL, ver->major, ver->minor, ver->patch);
        return;
    }

    if (ver->patch != MIX_PATCHLEVEL)
    {
        Printf(PRINT_WARNING, "I_InitSound: SDL_mixer version warning (%d.%d.%d vs %d.%d.%d dll)\n", MIX_MAJOR_VERSION,
               MIX_MINOR_VERSION, MIX_PATCHLEVEL, ver->major, ver->minor, ver->patch);
    }

    Printf(PRINT_HIGH, "I_InitSound: Initializing SDL_mixer\n");

#ifdef SDL20
    // Apparently, when Mix_OpenAudio requests a certain number of channels
    // and the device claims to not support that number of channels, instead
    // of handling it automatically behind the scenes, Mixer might initialize
    // with a broken audio buffer instead.  Using this function instead works
    // around the problem.
    if (Mix_OpenAudioDevice((int32_t)snd_samplerate, AUDIO_S16SYS, 2, 1024, NULL, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE) < 0)
#endif
    {
        Printf(PRINT_ERROR, "I_InitSound: Error initializing SDL_mixer: %s\n", Mix_GetError());
        return;
    }

    if (!Mix_QuerySpec(&mixer_freq, &mixer_format, &mixer_channels))
    {
        Printf(PRINT_ERROR, "I_InitSound: Error initializing SDL_mixer: %s\n", Mix_GetError());
        return;
    }

    Printf("I_InitSound: Using %d channels (freq:%d, fmt:%d, chan:%d)\n", Mix_AllocateChannels(NUM_CHANNELS),
           mixer_freq, mixer_format, mixer_channels);

    sound_initialized = true;

    SDL_PauseAudio(0);

    Printf("I_InitSound: sound module ready\n");

    I_InitMusic();

    // Half of fix for stopping wrong sound, these need to be false
    // to be regarded as empty (they'd be initialised to something weird)
    for (int32_t i = 0; i < NUM_CHANNELS; i++)
        channel_in_use[i] = false;
}

void I_ShutdownSound(void)
{
    if (!sound_initialized)
        return;

    I_ShutdownMusic();

    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

VERSION_CONTROL(i_sound_cpp, "$Id: e4105f5c226a8c1a8d298cf3ee910a59ec303b23 $")
