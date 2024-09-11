//----------------------------------------------------------------------------
//  EDGE Music handling Code
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
// -ACB- 1999/11/13 Written
//

#include "s_music.h"

#include <stdlib.h>

#include "ddf_main.h"
#include "dm_state.h"
#include "epi_filesystem.h"
#include "epi_str_util.h"
#include "i_system.h"
#include "m_misc.h"
#include "s_fluid.h"
#include "s_ogg.h"
#include "s_sound.h"
#include "snd_types.h"
#include "w_epk.h"
#include "w_files.h"


// music slider value
EDGE_DEFINE_CONSOLE_VARIABLE(music_volume, "0.15", kConsoleVariableFlagArchive)

bool no_music = false;

// Current music handle
static AbstractMusicPlayer *music_player;

std::string entry_playing = "";
static bool entry_looped;

void ChangeMusic(std::string_view song_name, bool loop)
{
    if (no_music)
        return;

    if (song_name.empty())
    {
        LogWarning("ChangeMusic: no song name given.\n");
        return;
    }

    // Consier "NONE" or "STOP" verbatim as directives
    if (song_name == "NONE" || song_name == "STOP")
    {
        StopMusic();
        return;
    }

    // -AJA- don't restart the current song (DOOM compatibility)
    // TODO: Is this still the behavior we want? - Dasho
    if (song_name == entry_playing && entry_looped)
        return;

    StopMusic();

    entry_playing = song_name;
    entry_looped  = loop;

    // open the file or lump, and read it into memory
    epi::File *F = OpenPackFile(entry_playing, "music");
    if (!F)
    {
        LogWarning("ChangeMusic: music entry '%s' not found.\n", entry_playing.c_str());
        return;
    }

    SoundFormat fmt = kSoundUnknown;

    // for FILE and PACK, use the file extension
    fmt = SoundFilenameToFormat(song_name);

    // NOTE: players are responsible for deleting 'F'
    switch (fmt)
    {
    case kSoundOGG:
        music_player = PlayOGGMusic(F, loop);
        break;

    case kSoundMIDI:
        music_player = PlayFluidMusic(F, loop);
        break;

    default:
        delete F;
        LogPrint("ChangeMusic: unknown format\n");
        break;
    }
}

void ResumeMusic(void)
{
    if (music_player)
        music_player->Resume();
}

void PauseMusic(void)
{
    if (music_player)
        music_player->Pause();
}

void StopMusic(void)
{
    // You can't stop the rock!! This does...

    if (music_player)
    {
        music_player->Stop();
        delete music_player;
        music_player = nullptr;
    }

    entry_playing.clear();
    entry_looped  = false;
}

void MusicTicker(void)
{
    if (music_player)
        music_player->Ticker();
}
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
