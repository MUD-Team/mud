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

int         entry_playing = -1;
static bool entry_looped;

void ChangeMusic(int entry_number, bool loop)
{
    if (no_music)
        return;

    // -AJA- playlist number 0 reserved to mean "no music"
    if (entry_number <= 0)
    {
        StopMusic();
        return;
    }

    // -AJA- don't restart the current song (DOOM compatibility)
    if (entry_number == entry_playing && entry_looped)
        return;

    StopMusic();

    entry_playing = entry_number;
    entry_looped  = loop;

    // when we cannot find the music entry, no music will play
    const PlaylistEntry *play = playlist.Find(entry_number);
    if (!play)
    {
        LogWarning("Could not find music entry [%d]\n", entry_number);
        return;
    }

    // open the file or lump, and read it into memory
    epi::File *F = OpenPackFile(play->info_, "");
    if (!F)
    {
        LogWarning("ChangeMusic: pack entry '%s' not found.\n", play->info_.c_str());
        return;
    }

    SoundFormat fmt = kSoundUnknown;

    // for FILE and PACK, use the file extension
    fmt = SoundFilenameToFormat(play->info_);

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

    entry_playing = -1;
    entry_looped  = false;
}

void MusicTicker(void)
{
    if (music_player)
        music_player->Ticker();
}
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
