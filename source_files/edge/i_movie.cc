//----------------------------------------------------------------------------
//  EDGE Movie Playback (MPEG)
//----------------------------------------------------------------------------
//
//  Copyright (c) 2018-2024 The EDGE Team
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
//  Adapted from the EDGE 2.x RoQ/FFMPEG implementation
//----------------------------------------------------------------------------

#include "epi.h"
#include "i_defs_gl.h"
#include "i_sound.h"
#include "i_system.h"
#include "pl_mpeg.h"
#include "r_gldefs.h"
#include "r_modes.h"
#include "s_blit.h"
#include "s_music.h"
#include "s_sound.h"
#include "sokol_color.h"
#include "w_files.h"

extern bool sound_device_stereo;
extern int  sound_device_frequency;

bool                    playing_movie;
static bool             need_canvas_update;
static bool             skip_bar_active;
static GLuint           canvas             = 0;
static uint8_t         *rgb_data           = nullptr;
static plm_t           *decoder            = nullptr;
static int              movie_sample_rate  = 0;
static float            skip_time;
static uint8_t         *movie_bytes        = nullptr;

static bool MovieSetupAudioStream(int rate)
{
    plm_set_audio_lead_time(decoder, (double)1024 / ((double)rate));
    movie_sample_rate = rate;
    PauseMusic();
    // Need to flush Queue to keep movie audio/video from desyncing initially
    SoundQueueStop();
    SoundQueueInitialize();

    return true;
}

void MovieAudioCallback(plm_t *mpeg, plm_samples_t *samples, void *user)
{
    (void)mpeg;
    (void)user;
    SoundData *movie_buf = SoundQueueGetFreeBuffer(PLM_AUDIO_SAMPLES_PER_FRAME, sound_device_stereo ? kMixInterleaved : kMixMono);
    if (movie_buf)
    {
        movie_buf->length_ = PLM_AUDIO_SAMPLES_PER_FRAME;
        if (sound_device_stereo)
        {
            memcpy(movie_buf->data_, samples->interleaved, PLM_AUDIO_SAMPLES_PER_FRAME * 2 * sizeof(float));
            SoundQueueAddBuffer(movie_buf, movie_sample_rate);
        }
        else
        {
            float *src = samples->interleaved;
            float *dest = movie_buf->data_;
            float *dest_end = movie_buf->data_ + PLM_AUDIO_SAMPLES_PER_FRAME;
            for (; dest < dest_end; src += 2)
            {
                *dest++ = (src[0] + src[1]) * 0.5f;
            }
            SoundQueueAddBuffer(movie_buf, movie_sample_rate);
        }
    }
}

void MovieVideoCallback(plm_t *mpeg, plm_frame_t *frame, void *user)
{
    (void)mpeg;
    (void)user;

    plm_frame_to_rgb(frame, rgb_data, frame->width * 3);

    glBindTexture(GL_TEXTURE_2D, canvas);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame->width, frame->height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_data);
    need_canvas_update = true;
}

void PlayMovie(const std::string &name)
{
#ifdef SOKOL_DISABLED
    MovieDefinition *movie = moviedefs.Lookup(name.c_str());

    if (!movie)
    {
        LogWarning("PlayMovie: Movie definition %s not found!\n", name.c_str());
        return;
    }

    playing_movie      = false;
    need_canvas_update = false;
    skip_bar_active    = false;
    skip_time          = 0;

    int      length = 0;

    epi::File *mf = OpenFileFromPack(movie->info_.c_str());
    if (mf)
    {
        movie_bytes  = mf->LoadIntoMemory();
        length = mf->GetLength();
    }
    delete mf;

    if (!movie_bytes)
    {
        LogWarning("PlayMovie: Could not open %s!\n", movie->info_.c_str());
        return;
    }

    if (decoder)
    {
        plm_destroy(decoder);
        decoder = nullptr;
    }

    decoder = plm_create_with_memory(movie_bytes, length, FALSE);

    if (!decoder)
    {
        LogWarning("PlayMovie: Could not open %s!\n", name.c_str());
        delete[] movie_bytes;
        movie_bytes = nullptr;
        return;
    }

    if (!no_sound && !(movie->special_ & kMovieSpecialMute) && plm_get_num_audio_streams(decoder) > 0)
    {
        if (!MovieSetupAudioStream(plm_get_samplerate(decoder)))
        {
            plm_destroy(decoder);
            delete[] movie_bytes;
            movie_bytes = nullptr;
            decoder = nullptr;
            return;
        }
    }

    if (canvas)
        glDeleteTextures(1, &canvas);

    glGenTextures(1, &canvas);

    if (rgb_data)
    {
        delete[] rgb_data;
        rgb_data = nullptr;
    }

    int   movie_width  = plm_get_width(decoder);
    int   movie_height = plm_get_height(decoder);
    float movie_ratio  = (float)movie_width / movie_height;
    // Size frame using DDFMOVIE scaling selection
    // Should only need to be set once unless at some point
    // we allow menu access/console while a movie is playing
    int   frame_height = 0;
    int   frame_width  = 0;
    float tx1          = 0.0f;
    float tx2          = 1.0f;
    float ty1          = 0.0f;
    float ty2          = 1.0f;
    if (movie->scaling_ == kMovieScalingAutofit)
    {
        // If movie and display ratios match (ish), stretch it
        if (fabs((float)current_screen_width / current_screen_height / movie_ratio - 1.0f) <= 0.10f)
        {
            frame_height = current_screen_height;
            frame_width  = current_screen_width;
        }
        else // Zoom
        {
            frame_height = current_screen_height;
            frame_width  = RoundToInteger((float)current_screen_height * movie_ratio);
        }
    }
    else if (movie->scaling_ == kMovieScalingNoScale)
    {
        frame_height = movie_height;
        frame_width  = movie_width;
    }
    else if (movie->scaling_ == kMovieScalingZoom)
    {
        frame_height = current_screen_height;
        frame_width  = RoundToInteger((float)current_screen_height * movie_ratio);
    }
    else // Stretch, aspect ratio gets BTFO potentially
    {
        frame_height = current_screen_height;
        frame_width  = current_screen_width;
    }

    int vx1 = current_screen_width / 2 - frame_width / 2;
    int vx2 = current_screen_width / 2 + frame_width / 2;
    int vy1 = current_screen_height / 2 + frame_height / 2;
    int vy2 = current_screen_height / 2 - frame_height / 2;

    int num_pixels = movie_width * movie_height * 3;
    rgb_data       = new uint8_t[num_pixels];
    memset(rgb_data, 0, num_pixels);
    plm_set_video_decode_callback(decoder, MovieVideoCallback, nullptr);
    plm_set_audio_decode_callback(decoder, MovieAudioCallback, nullptr);
    if (!no_sound)
    {
        plm_set_audio_enabled(decoder, TRUE);
        plm_set_audio_stream(decoder, 0);
    }
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    FinishFrame();
    StartFrame();
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    FinishFrame();

    SetupMatrices2D();

    double last_time = (double)SDL_GetTicks() / 1000.0;

    playing_movie = true;

    while (playing_movie)
    {
        if (plm_has_ended(decoder))
        {
            playing_movie = false;
            break;
        }

        double current_time = (double)SDL_GetTicks() / 1000.0;
        double elapsed_time = current_time - last_time;
        if (elapsed_time > 1.0 / 30.0)
            elapsed_time = 1.0 / 30.0;
        last_time = current_time;

        plm_decode(decoder, elapsed_time);

        if (need_canvas_update)
        {
            StartFrame();

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, canvas);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glDisable(GL_ALPHA_TEST);

            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

            glBegin(GL_QUADS);

            glTexCoord2f(tx1, ty2);
            glVertex2i(vx1, vy2);

            glTexCoord2f(tx2, ty2);
            glVertex2i(vx2, vy2);

            glTexCoord2f(tx2, ty1);
            glVertex2i(vx2, vy1);

            glTexCoord2f(tx1, ty1);
            glVertex2i(vx1, vy1);

            glEnd();

            glDisable(GL_TEXTURE_2D);

            // Fade-in
            float fadein = plm_get_time(decoder);
            if (fadein <= 0.25f)
            {
                glColor4f(0, 0, 0, (0.25f - fadein) / 0.25f);
                glEnable(GL_BLEND);

                glBegin(GL_QUADS);

                glVertex2i(vx1, vy2);
                glVertex2i(vx2, vy2);
                glVertex2i(vx2, vy1);
                glVertex2i(vx1, vy1);

                glEnd();

                glDisable(GL_BLEND);
            }

            if (skip_bar_active)
            {
                // Draw black box at bottom of screen
                HUDSolidBox(hud_x_left, 196, hud_x_right, 200, SG_BLACK_RGBA32);

                // Draw progress
                HUDSolidBox(hud_x_left, 197, hud_x_right * (skip_time / 0.9f), 199, SG_WHITE_RGBA32);
            }

            FinishFrame();

            need_canvas_update = false;
        }

        /* check if press key/button */
        SDL_Event sdl_ev;
        while (SDL_PollEvent(&sdl_ev))
        {
            switch (sdl_ev.type)
            {
            case SDL_KEYDOWN:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_CONTROLLERBUTTONDOWN:
                skip_bar_active = true;
                break;

            case SDL_KEYUP:
            case SDL_MOUSEBUTTONUP:
            case SDL_CONTROLLERBUTTONUP:
                skip_bar_active = false;
                skip_time       = 0;
                break;

            default:
                break;
            }
        }
        if (skip_bar_active)
        {
            skip_time += elapsed_time;
            if (skip_time > 1)
                playing_movie = false;
        }
    }
    last_time      = (double)SDL_GetTicks() / 1000.0;
    double fadeout = 0;
    while (fadeout <= 0.25f)
    {
        double current_time = (double)SDL_GetTicks() / 1000.0;
        fadeout             = current_time - last_time;
        StartFrame();

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, canvas);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glDisable(GL_ALPHA_TEST);

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);

        glTexCoord2f(tx1, ty2);
        glVertex2i(vx1, vy2);

        glTexCoord2f(tx2, ty2);
        glVertex2i(vx2, vy2);

        glTexCoord2f(tx2, ty1);
        glVertex2i(vx2, vy1);

        glTexCoord2f(tx1, ty1);
        glVertex2i(vx1, vy1);

        glEnd();

        glDisable(GL_TEXTURE_2D);

        // Fade-out
        glColor4f(0, 0, 0, GLM_MAX(0.0f, 1.0f - ((0.25f - fadeout) / 0.25f)));
        glEnable(GL_BLEND);

        glBegin(GL_QUADS);

        glVertex2i(vx1, vy2);
        glVertex2i(vx2, vy2);
        glVertex2i(vx2, vy1);
        glVertex2i(vx1, vy1);

        glEnd();

        glDisable(GL_BLEND);

        FinishFrame();
    }
    plm_destroy(decoder);
    decoder = nullptr;
    delete[] movie_bytes;
    movie_bytes = nullptr;
    if (rgb_data)
    {
        delete[] rgb_data;
        rgb_data = nullptr;
    }
    if (canvas)
    {
        glDeleteTextures(1, &canvas);
        canvas = 0;
    }
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    FinishFrame();
    StartFrame();
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    FinishFrame();
    ResumeMusic();
    return;
#endif
}