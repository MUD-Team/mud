// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
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
//  Plays music utilizing the FluidLite music library.
//
//-----------------------------------------------------------------------------

#include "i_musicsystem_fluidlite.h"

#include <SDL_mixer.h>
#include <fluidlite.h>
#include <math.h>

#include "i_midi.h"
#include "i_system.h"
#include "m_fileio.h"
#include "odamex.h"
#include "physfs.h"

EXTERN_CVAR(snd_samplerate)
EXTERN_CVAR(snd_soundfont)

// ============================================================================
// Partially based on an implementation from prboom-plus by Nicholai Main (Natt).
// ============================================================================

static void fluidError(int level, char *message, void *data)
{
    (void)level;
    (void)data;
    I_FatalError("Fluidlite: %s\n", message);
}

static void *fluidFileOpen(fluid_fileapi_t *fileapi, const char *filename)
{
    PHYSFS_File *fp = PHYSFS_openRead(filename);
    if (!fp)
        return nullptr;
    return fp;
}

static int fluidFileClose(void *handle)
{
    int res = PHYSFS_close((PHYSFS_File *)handle);
    if (res == 0)
        return -1; // FLUID_FAILED
    else
        return 0; // FLUID_OK
}

static int fluidFileSeek(void* handle, long offset, int origin)
{
    long real_offset = offset;
    if (origin == SEEK_CUR)
    {
        real_offset = PHYSFS_tell((PHYSFS_File *)handle) + offset;
    }
    if (origin == SEEK_END)
    {
        real_offset = PHYSFS_fileLength((PHYSFS_File *)handle) - offset;
    }
    int res = PHYSFS_seek((PHYSFS_File *)handle, real_offset);
    if (res == 0)
        return -1; // FLUID_FAILED
    else
        return 0; // FLUID_OK
}

static long fluidFileTell(void* handle)
{
    long res = PHYSFS_tell((PHYSFS_File *)handle);
    if (res == -1)
        return -1; // FLUID_FAILED
    else
        return res;
}

static int fluidFileRead(void *buf, int count, void* handle)
{
    int res = PHYSFS_readBytes((PHYSFS_File *)handle, buf, (PHYSFS_uint64)count);
    if (res == count)
        return 0; // FLUID_OK
    else
        return -1; // FLUID_FAILED
}

static void rtNoteOn(void *userdata, uint8_t channel, uint8_t note, uint8_t velocity)
{
    fluid_synth_noteon((fluid_synth_t *)userdata, channel, note, velocity);
}

static void rtNoteOff(void *userdata, uint8_t channel, uint8_t note)
{
    fluid_synth_noteoff((fluid_synth_t *)userdata, channel, note);
}

static void rtNoteAfterTouch(void *userdata, uint8_t channel, uint8_t note, uint8_t atVal)
{
    fluid_synth_key_pressure((fluid_synth_t *)userdata, channel, note, atVal);
}

static void rtChannelAfterTouch(void *userdata, uint8_t channel, uint8_t atVal)
{
    fluid_synth_channel_pressure((fluid_synth_t *)userdata, channel, atVal);
}

static void rtControllerChange(void *userdata, uint8_t channel, uint8_t type, uint8_t value)
{
    fluid_synth_cc((fluid_synth_t *)userdata, channel, type, value);
}

static void rtPatchChange(void *userdata, uint8_t channel, uint8_t patch)
{
    fluid_synth_program_change((fluid_synth_t *)userdata, channel, patch);
}

static void rtPitchBend(void *userdata, uint8_t channel, uint8_t msb, uint8_t lsb)
{
    fluid_synth_pitch_bend((fluid_synth_t *)userdata, channel, (msb << 7) | lsb);
}

static void rtSysEx(void *userdata, const uint8_t *msg, size_t size)
{
    fluid_synth_sysex((fluid_synth_t *)userdata, (const char *)msg, (int)size, nullptr, nullptr, nullptr, 0);
}

static void rtDeviceSwitch(void *userdata, size_t track, const char *data, size_t length)
{
    (void)userdata;
    (void)track;
    (void)data;
    (void)length;
}

static size_t rtCurrentDevice(void *userdata, size_t track)
{
    (void)userdata;
    (void)track;
    return 0;
}

static void playSynth(void *userdata, uint8_t *stream, size_t length)
{
    fluid_synth_write_s16((fluid_synth_t *)userdata, (int)length / 4, stream, 0, 2, stream + 2, 0, 2);
}

static void fluidPlaybackHook(void *udata, uint8_t *stream, int len)
{
    FluidLiteMusicSystem *player = (FluidLiteMusicSystem *)udata;
    if (player->isPlaying() && !player->isPaused())
    {
        if (player->m_sequencer->positionAtEnd())
        {
            if (player->isLooping())
                player->m_sequencer->Rewind();
            else
                return;
        }
        player->m_sequencer->playStream(stream, len);
    }
}

FluidLiteMusicSystem::FluidLiteMusicSystem()
    : m_synth(NULL), m_synthSettings(NULL), m_soundfontLoader(NULL), m_isInitialized(false), m_sequencer(NULL),
      m_interface(NULL), m_loopSong(false)
{
    // Minimize log spam, but allow Fluidlite panics to bubble up
    fluid_set_log_function(FLUID_PANIC, fluidError, nullptr);
    fluid_set_log_function(FLUID_ERR, nullptr, nullptr);
    fluid_set_log_function(FLUID_WARN, nullptr, nullptr);
    fluid_set_log_function(FLUID_DBG, nullptr, nullptr);

    // Possibly expose other settings via CVAR - Dasho
    m_synthSettings = new_fluid_settings();
    fluid_settings_setstr(m_synthSettings, "synth.reverb.active", "no");
    fluid_settings_setstr(m_synthSettings, "synth.chorus.active", "no");
    fluid_settings_setnum(m_synthSettings, "synth.sample-rate", (int)snd_samplerate);
    fluid_settings_setnum(m_synthSettings, "synth.polyphony", 64);

    m_synth = new_fluid_synth(m_synthSettings);

    m_soundfontLoader          = new_fluid_defsfloader();
    m_soundfontLoader->fileapi = (fluid_fileapi_t *)calloc(1, sizeof(fluid_fileapi_t));
    fluid_init_default_fileapi(m_soundfontLoader->fileapi);
    m_soundfontLoader->fileapi->fopen = fluidFileOpen;
    m_soundfontLoader->fileapi->fclose = fluidFileClose;
    m_soundfontLoader->fileapi->fseek = fluidFileSeek;
    m_soundfontLoader->fileapi->fread = fluidFileRead;
    m_soundfontLoader->fileapi->ftell = fluidFileTell;
    fluid_synth_add_sfloader(m_synth, m_soundfontLoader);

    if (fluid_synth_sfload(m_synth, std::string("soundfonts/").append(snd_soundfont.cstring()).c_str(),
                           1) == -1)
    {
        Printf(PRINT_WARNING, "I_InitMusic: FluidLite Initialization failure.\n");
        delete_fluid_synth(m_synth);
        m_synth = NULL;
        delete_fluid_settings(m_synthSettings);
        m_synthSettings = NULL;
        m_isInitialized = false;
        return;
    }

    fluid_synth_program_reset(m_synth);

    m_sequencer = new MidiSequencer;
    m_interface = new MidiRealTimeInterface;
    memset(m_interface, 0, sizeof(MidiRealTimeInterface));

    m_interface->rtUserData           = m_synth;
    m_interface->rt_noteOn            = rtNoteOn;
    m_interface->rt_noteOff           = rtNoteOff;
    m_interface->rt_noteAfterTouch    = rtNoteAfterTouch;
    m_interface->rt_channelAfterTouch = rtChannelAfterTouch;
    m_interface->rt_controllerChange  = rtControllerChange;
    m_interface->rt_patchChange       = rtPatchChange;
    m_interface->rt_pitchBend         = rtPitchBend;
    m_interface->rt_systemExclusive   = rtSysEx;

    m_interface->onPcmRender          = playSynth;
    m_interface->onPcmRender_userdata = m_synth;

    m_interface->pcmSampleRate = (int)snd_samplerate;
    m_interface->pcmFrameSize  = 2 /*channels*/ * 2 /*size of one sample*/;

    m_interface->rt_deviceSwitch  = rtDeviceSwitch;
    m_interface->rt_currentDevice = rtCurrentDevice;

    m_sequencer->setInterface(m_interface);

    Printf(PRINT_HIGH, "I_InitMusic: Music playback enabled using FluidLite.\n");
    m_isInitialized = true;
}

FluidLiteMusicSystem::~FluidLiteMusicSystem()
{
    if (!isInitialized())
        return;

    if (isPlaying())
        stopSong();

    m_isInitialized = false;

    if (m_synth)
    {
        delete_fluid_synth(m_synth);
        m_synth = NULL;
    }
    if (m_synthSettings)
    {
        delete_fluid_settings(m_synthSettings);
        m_synthSettings = NULL;
    }
    if (m_sequencer)
    {
        delete m_sequencer;
        m_sequencer = NULL;
    }
    if (m_interface)
    {
        delete m_interface;
        m_interface = NULL;
    }
}

void FluidLiteMusicSystem::setVolume(float volume)
{
    MusicSystem::setVolume(volume);

    if (!isInitialized() || !isPlaying())
        return;

    // See if this needs to be scaled against the normal volume value - Dasho
    fluid_synth_set_gain(m_synth, volume);
}

void FluidLiteMusicSystem::startSong(byte *data, size_t length, bool loop)
{
    if (!isInitialized())
        return;
    if (isPlaying())
    {
        stopSong();
    }
    if (m_sequencer->loadMidi(data, length))
    {
        MusicSystem::startSong(data, length, loop);
        m_loopSong = loop;
        Mix_HookMusic(fluidPlaybackHook, this);
    }
}

void FluidLiteMusicSystem::stopSong()
{
    MusicSystem::stopSong();
    if (isInitialized())
        fluid_synth_system_reset(m_synth);
    Mix_HookMusic(NULL, NULL);
}

void FluidLiteMusicSystem::pauseSong()
{
    MusicSystem::pauseSong();
}

void FluidLiteMusicSystem::resumeSong()
{
    MusicSystem::resumeSong();
}