//----------------------------------------------------------------------------
//  EDGE Sound System for Sokol
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

#include "i_sound.h"

#include "epi.h"
#include "epi_filesystem.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "s_blit.h"
#include "s_cache.h"
#include "s_fluid.h"
#include "s_sound.h"
#define SOKOL_AUDIO_IMPL
#define SOKOL_ASSERT(c) EPI_ASSERT(c)
#include "sokol_audio.h"
#include "sokol_log.h"

static saudio_desc sound_device_check;
static bool sound_initialized;

// If true, sound system is off/not working. Changed to false if sound init ok.
bool no_sound = false;

int  sound_device_frequency;
int  sound_device_bytes_per_sample;
int  sound_device_samples_per_buffer;
bool sound_device_stereo;

static bool audio_is_locked = true;

std::vector<std::string> available_soundfonts;
extern std::string       game_directory;
extern std::string       home_directory;
extern ConsoleVariable   midi_soundfont;

void SoundFillCallback(float* buffer, int num_frames, int num_channels)
{
    memset(buffer, 0, num_frames * num_channels * sizeof(float));
    if (!audio_is_locked)
        MixAllSoundChannels(buffer, num_frames);
}

static bool TryOpenSound(int want_freq, bool want_stereo)
{
    sound_device_check.logger.func = slog_func;
    sound_device_check.stream_cb = SoundFillCallback;
    sound_device_check.num_channels = want_stereo ? 2 : 1;
    sound_device_check.sample_rate = want_freq;
    sound_device_check.buffer_frames = 1024;
    saudio_setup(&sound_device_check);

    LogPrint("StartupSound: trying %d Hz %s\n", want_freq, want_stereo ? "Stereo" : "Mono");

    sound_initialized = true;

    return true;
}

void StartupAudio(void)
{
    if (no_sound)
        return;

    int  want_freq   = 44100;
    bool want_stereo = (var_sound_stereo >= 1);

    if (FindArgument("mono") > 0)
        want_stereo = false;
    if (FindArgument("stereo") > 0)
        want_stereo = true;

    bool success = false;

    if (TryOpenSound(want_freq, want_stereo))
        success = true;

    if (!success)
    {
        LogPrint("StartupSound: Unable to find a working sound mode!\n");
        no_sound = true;
        return;
    }

    if (want_stereo && sound_device_check.num_channels != 2)
        LogPrint("StartupSound: stereo sound not available.\n");
    else if (!want_stereo && sound_device_check.num_channels != 1)
        LogPrint("StartupSound: mono sound not available.\n");

    if (sound_device_check.sample_rate != want_freq)
    {
        LogPrint("StartupSound: %d Hz sound not available.\n", want_freq);
    }

    sound_device_bytes_per_sample   = sizeof(float); // keep this line in case we ever change audio backends or this size becomes variable
    sound_device_samples_per_buffer = sound_device_check.buffer_frames;

    EPI_ASSERT(sound_device_bytes_per_sample > 0);
    EPI_ASSERT(sound_device_samples_per_buffer > 0);

    sound_device_frequency = sound_device_check.sample_rate;
    sound_device_stereo    = (sound_device_check.num_channels == 2);

    // update Sound Options menu
    if (sound_device_stereo != (var_sound_stereo >= 1))
        var_sound_stereo = sound_device_stereo ? 1 : 0;

    // display some useful stuff
    LogPrint("StartupSound: Success @ %d Hz %s\n", sound_device_frequency, sound_device_stereo ? "Stereo" : "Mono");

    return;
}

void ShutdownAudio(void)
{
    if (no_sound)
        return;

    StopMusic();        

    LockAudio();

    SoundQueueShutdown();

    FreeSoundChannels();

    if (sound_initialized)
    {
        saudio_shutdown();
        sound_initialized = false;
    }
}

void LockAudio(void)
{
    audio_is_locked = true;
}

void UnlockAudio(void)
{
    audio_is_locked = false;
}

void StartupMusic(void)
{
    // Set default SF2 location in CVAR if needed
    if (midi_soundfont.s_.empty())
        midi_soundfont = "Default.sf2";

    char **got_names = PHYSFS_enumerateFiles("soundfont");

    // seems this only happens on out-of-memory error
    if (!got_names)
    {
        LogPrint("StartupMusic: Error reading soundfont directory; MIDI playback will be disabled.\n");
        fluid_disabled = true;
        return;
    }

    char **p;
    PHYSFS_Stat statter;

    for (p = got_names; *p; p++)
    {
        if (PHYSFS_stat(epi::PathAppend("soundfont", *p).c_str(), &statter) == 0)
        {
            LogPrint("Could not stat %s: %s\n", *p, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            continue;
        }

        if (statter.filetype == PHYSFS_FILETYPE_REGULAR && epi::GetExtension(*p) == ".sf2")
            available_soundfonts.push_back(*p);
    }

    PHYSFS_freeList(got_names);

    if (available_soundfonts.empty() || !StartupFluid())
        fluid_disabled = true;

    return;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
