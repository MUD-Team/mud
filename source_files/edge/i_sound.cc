//----------------------------------------------------------------------------
//  EDGE Sound System for SDL
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
#include "epi_file.h"
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
#include "w_wad.h"

// If true, sound system is off/not working. Changed to false if sound init ok.
bool no_sound = false;

SDL_AudioStream     *current_sound_device = nullptr;

int  sound_device_frequency;
int  sound_device_bytes_per_sample;
bool sound_device_stereo;

static bool audio_is_locked = false;

std::vector<std::string> available_soundfonts;
extern std::string       game_directory;
extern std::string       home_directory;
extern ConsoleVariable   midi_soundfont;

static uint8_t *sdl3_mix_buffer = nullptr;
static int sdl3_mix_size = 0;

void SoundFillCallback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
    (void)userdata;
    if (!sdl3_mix_buffer)
    {
        sdl3_mix_buffer = (uint8_t *)malloc(additional_amount);
        sdl3_mix_size = additional_amount;
    }
    else if (sdl3_mix_size < additional_amount)
    {
        sdl3_mix_buffer = (uint8_t *)realloc(sdl3_mix_buffer, additional_amount);
        sdl3_mix_size = additional_amount;
    }
    memset(sdl3_mix_buffer, 0, additional_amount);
    MixAllSoundChannels(sdl3_mix_buffer, additional_amount);
    SDL_PutAudioStreamData(current_sound_device, sdl3_mix_buffer, additional_amount);
}

static bool TryOpenSound(int want_freq, bool want_stereo)
{
    SDL_AudioSpec trydev;
    SDL_zero(trydev);

    LogPrint("StartupSound: trying %d Hz %s\n", want_freq, want_stereo ? "Stereo" : "Mono");

    trydev.freq     = want_freq;
    trydev.format   = SDL_AUDIO_S16;
    trydev.channels = want_stereo ? 2 : 1;

    current_sound_device = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &trydev, SoundFillCallback, nullptr);

    if (current_sound_device)
        return true;

    LogPrint("  failed: %s\n", SDL_GetError());

    return false;
}

void StartupAudio(void)
{
    if (no_sound)
        return;

    std::string driver = ArgumentValue("audiodriver");

    if (driver.empty())
    {
        const char *check = SDL_getenv("SDL_AUDIODRIVER");
        if (check)
            driver = check;
    }

    if (driver.empty())
        driver = "default";

    if (epi::StringCaseCompareASCII(driver, "default") != 0)
    {
        SDL_Environment *enviro = SDL_GetEnvironment();
        if (enviro)
            SDL_SetEnvironmentVariable(enviro, "SDL_AUDIODRIVER", driver.c_str(), true);
    }

    LogPrint("SDL_Audio_Driver: %s\n", driver.c_str());

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        LogPrint("StartupSound: Couldn't init SDL AUDIO! %s\n", SDL_GetError());
        no_sound = true;
        return;
    }

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

    sound_device_bytes_per_sample   = 2 * (want_stereo ? 2 : 1);

    EPI_ASSERT(sound_device_bytes_per_sample > 0);

    sound_device_frequency = want_freq;
    sound_device_stereo    = want_stereo;

    // update Sound Options menu
    if (sound_device_stereo != (var_sound_stereo >= 1))
        var_sound_stereo = sound_device_stereo ? 1 : 0;

    // display some useful stuff
    LogPrint("StartupSound: Success @ %d Hz %s\n", sound_device_frequency, sound_device_stereo ? "Stereo" : "Mono");

    return;
}

void AudioShutdown(void)
{
    if (no_sound)
        return;

    ShutdownSound();

    no_sound = true;

    SDL_DestroyAudioStream(current_sound_device);
}

void LockAudio(void)
{
    if (audio_is_locked)
    {
        UnlockAudio();
        FatalError("LockAudio: called twice without unlock!\n");
    }

    SDL_LockAudioStream(current_sound_device);
    audio_is_locked = true;
}

void UnlockAudio(void)
{
    if (audio_is_locked)
    {
        SDL_UnlockAudioStream(current_sound_device);
        audio_is_locked = false;
    }
}

void StartupMusic(void)
{
    // Check for soundfonts and instrument banks
    std::vector<epi::DirectoryEntry> sfd;
    std::string                      soundfont_dir = epi::PathAppend(game_directory, "soundfont");

    // Set default SF2 location in CVAR if needed
    if (midi_soundfont.s_.empty())
        midi_soundfont = epi::SanitizePath(epi::PathAppend(soundfont_dir, "Default.sf2"));

    if (!ReadDirectory(sfd, soundfont_dir, "*.sf2"))
    {
        LogWarning("StartupMusic: Failed to read '%s' directory!\n", soundfont_dir.c_str());
    }
    else
    {
        for (size_t i = 0; i < sfd.size(); i++)
        {
            if (!sfd[i].is_dir)
                available_soundfonts.push_back(epi::SanitizePath(sfd[i].name));
        }
    }

    if (home_directory != game_directory)
    {
        // Check home_directory soundfont folder as well; create it if it
        // doesn't exist (home_directory only)
        sfd.clear();
        soundfont_dir = epi::PathAppend(home_directory, "soundfont");
        if (!epi::IsDirectory(soundfont_dir))
            epi::MakeDirectory(soundfont_dir);

        if (!ReadDirectory(sfd, soundfont_dir, "*.sf2"))
        {
            LogWarning("StartupMusic: Failed to read '%s' directory!\n", soundfont_dir.c_str());
        }
        else
        {
            for (size_t i = 0; i < sfd.size(); i++)
            {
                if (!sfd[i].is_dir)
                    available_soundfonts.push_back(epi::SanitizePath(sfd[i].name));
            }
        }
    }

    if (!StartupFluid())
        fluid_disabled = true;

    return;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
