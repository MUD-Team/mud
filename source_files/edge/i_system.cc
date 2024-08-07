//----------------------------------------------------------------------------
//  EDGE Misc System Interface Code
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

#include "i_system.h"

#include <chrono>
#include <thread>
#include <stdarg.h>

#include "con_main.h"
#include "dm_defs.h"
#include "e_main.h"
#include "epi.h"
#include "epi_filesystem.h"
#include "epi_str_util.h"
#include "epi_windows.h"
#include "g_game.h"
#include "m_argv.h"
#include "m_misc.h"
#include "physfs.h"
#include "s_sound.h"
#include "sokol_time.h"
#include "version.h"

extern epi::File *log_file;

#if !defined(__MINGW32__) && (defined(WIN32) || defined(_WIN32) || defined(_WIN64))
extern HANDLE windows_timer;
#endif

// output string buffer
static constexpr int16_t kMessageBufferSize = 4096;
static char              message_buffer[kMessageBufferSize];

void SystemStartup(void)
{
    StartupGraphics(); // SDL requires this to be called first
    StartupControl();
    StartupAudio();
}

void CloseProgram(int exitnum)
{
    exit(exitnum);
}

void LogWarning(const char *warning, ...)
{
    va_list argptr;

    va_start(argptr, warning);
    vsprintf(message_buffer, warning, argptr);
    va_end(argptr);

    LogPrint("WARNING: %s", message_buffer);
}

void FatalError(const char *error, ...)
{
    va_list argptr;

    va_start(argptr, error);
    vsprintf(message_buffer, error, argptr);
    va_end(argptr);

    if (log_file)
    {
        log_file->WriteString(epi::StringFormat("ERROR: %s\n", message_buffer));
    }

    SystemShutdown();

    ShowMessageBox(message_buffer, "EDGE-Classic Error");

    CloseProgram(EXIT_FAILURE);
}

void LogPrint(const char *message, ...)
{
    va_list argptr;

    char printbuf[kMessageBufferSize];
    printbuf[kMessageBufferSize - 1] = 0;

    va_start(argptr, message);
    vsnprintf(printbuf, sizeof(printbuf), message, argptr);
    va_end(argptr);

    EPI_ASSERT(printbuf[kMessageBufferSize - 1] == 0);

    if (log_file)
    {
        log_file->WriteString(printbuf);
    }

    // Send the message to the console.
    ConsolePrint("%s", printbuf);

    printf("%s", printbuf);
}

void ShowMessageBox(const char *message, const char *title)
{
// SOKOL_FIX
#ifdef WIN32
    // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, nullptr);
    MessageBoxA((HWND)sapp_win32_get_hwnd(), message, title, MB_OK);
#else
    printf("%s: %s", title, message);
#endif
}

int PureRandomNumber(void)
{
    int P1 = (int)time(nullptr);
    int P2 = (int)GetMicroseconds();

    return (P1 ^ P2) & 0x7FFFFFFF;
}

uint32_t GetMicroseconds(void)
{
    return (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void SleepForMilliseconds(int millisecs)
{
#if !defined(__MINGW32__) && (defined(WIN32) || defined(_WIN32) || defined(_WIN64))
    // On Windows use high resolution timer if available, the Sleep Win32 call
    // defaults to 15.6ms resolution and timeBeginPeriod is problematic
    if (windows_timer != nullptr)
    {
        LARGE_INTEGER due_time;
        due_time.QuadPart = -((LONGLONG)(millisecs * 1000000) / 100);
        if (SetWaitableTimerEx(windows_timer, &due_time, 0, nullptr, nullptr, nullptr, 0))
        {
            WaitForSingleObject(windows_timer, INFINITE);
        }
        return;
    }
#endif

    static bool has_warned = false;
    if (!has_warned)
    {
        has_warned = true;
        LogWarning("SleepForMilliseconds: using busy wait on platform, please fix\n");
    }
    double now = stm_ms(stm_now());
    while (stm_ms(stm_now()) - now < millisecs)
    {
    }
}

void SystemShutdown(void)
{
    ShutdownAudio();
    ShutdownControl();
    ShutdownGraphics();

    if (log_file)
    {
        delete log_file;
        log_file = nullptr;
    }

    PHYSFS_deinit();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
