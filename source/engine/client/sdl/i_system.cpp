// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 918a04f32bf429bb5c3ef5cbabc737323f5dc92b $
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
// DESCRIPTION:
//	System specific interface stuff.
//	[Odamex] Fitted to work with SDL
//
//-----------------------------------------------------------------------------

#ifdef OSX
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#include "win32inc.h"
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <mmsystem.h>
#include <process.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <winsock2.h>
#endif // WIN32
#ifdef UNIX
// for getuid and geteuid
#include <pwd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#endif
#ifndef __OpenBSD__
#include <sys/timeb.h>
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <limits>
#include <sstream>

#include "c_dispatch.h"
#include "cl_main.h"
#include "cmdlib.h"
#include "d_main.h"
#include "g_game.h"
#include "i_sdl.h"
#include "i_sound.h"
#include "i_system.h"
#include "i_video.h"
#include "i_video_sdl20.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_misc.h"
#include "mud_includes.h"
#include "ui_public.h"
#include "v_video.h"
#include "w_wad.h"

ticcmd_t  emptycmd;
ticcmd_t *I_BaseTiccmd(void)
{
    return &emptycmd;
}

/* [Russell] - Modified to accomodate a minimal allowable heap size */
// These values are in megabytes
size_t       def_heapsize = 128;
const size_t min_heapsize = 8;

// The size we got back from I_ZoneBase in megabytes
size_t got_heapsize = 0;

uint32_t LanguageIDs[4];

//
// I_MegabytesToBytes
//
// Returns the megabyte value of size in bytes
size_t I_MegabytesToBytes(size_t Megabytes)
{
    return (Megabytes * 1024 * 1024);
}

//
// I_BytesToMegabytes
//
// Returns the byte value of size in megabytes
size_t I_BytesToMegabytes(size_t Bytes)
{
    if (!Bytes)
        return 0;

    return (Bytes / 1024 / 1024);
}

//
// I_ZoneBase
//
// Allocates a portion of system memory for the Zone Memory Allocator, returns
// the 'size' of what it could allocate in its parameter
void *I_ZoneBase(size_t *size)
{
    void *zone = NULL;

    // User wanted a different default size
    const char *p = Args.CheckValue("-heapsize");

    if (p)
        def_heapsize = atoi(p);

    if (def_heapsize < min_heapsize)
        def_heapsize = min_heapsize;

    // Set the size
    *size = I_MegabytesToBytes(def_heapsize);

    // Allocate the def_heapsize, otherwise try to allocate a smaller amount
    while ((zone == NULL) && (*size >= I_MegabytesToBytes(min_heapsize)))
    {
        zone = malloc(*size);

        if (zone != NULL)
            break;

        *size -= I_MegabytesToBytes(1);
    }

    // Our heap size we received
    got_heapsize = I_BytesToMegabytes(*size);

    // Die if the system has insufficient memory
    if (got_heapsize < min_heapsize)
        I_Error("I_ZoneBase: Insufficient memory available! Minimum size "
                "is %lu MB but got %lu MB instead",
                min_heapsize, got_heapsize);

    return zone;
}

void I_BeginRead()
{
}

void I_EndRead(void)
{
}

//
// I_GetTime
//
// [SL] Retrieve an arbitrarily-based time from a high-resolution timer with
// nanosecond accuracy.
//
dtime_t I_GetTime()
{
#if defined OSX
    clock_serv_t    cclock;
    mach_timespec_t mts;

    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    return mts.tv_sec * 1000LL * 1000LL * 1000LL + mts.tv_nsec;

#elif defined UNIX
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL * 1000LL * 1000LL + ts.tv_nsec;

#elif defined WIN32
    static bool          initialized = false;
    static LARGE_INTEGER initial_count;
    static double        nanoseconds_per_count;
    static LARGE_INTEGER last_count;

    if (!initialized)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        nanoseconds_per_count = 1000.0 * 1000.0 * 1000.0 / double(freq.QuadPart);

        QueryPerformanceCounter(&initial_count);
        last_count = initial_count;

        initialized = true;
    }

    LARGE_INTEGER current_count;
    QueryPerformanceCounter(&current_count);

    // [SL] ensure current_count is a sane value
    // AMD dual-core CPUs and buggy BIOSes sometimes cause QPC
    // to return different values from different CPU cores,
    // which ruins our timing. We check that the new value is
    // at least equal to the last value and that the new value
    // isn't too far past the last value (1 frame at 10fps).

    const int64_t min_count = last_count.QuadPart;
    const int64_t max_count = last_count.QuadPart + nanoseconds_per_count * I_ConvertTimeFromMs(100);

    if (current_count.QuadPart < min_count || current_count.QuadPart > max_count)
        current_count = last_count;

    last_count = current_count;

    return nanoseconds_per_count * (current_count.QuadPart - initial_count.QuadPart);

#else
    // [SL] use SDL_GetTicks, but account for the fact that after
    // 49 days, it wraps around since it returns a 32-bit int32_t
    static const uint64_t mask         = 0xFFFFFFFFLL;
    static uint64_t       last_time    = 0LL;
    uint64_t              current_time = SDL_GetTicks();

    if (current_time < (last_time & mask)) // just wrapped around
        last_time += mask + 1 - (last_time & mask) + current_time;
    else
        last_time = current_time;

    return last_time * 1000000LL;

#endif
}

dtime_t I_MSTime()
{
    return I_ConvertTimeToMs(I_GetTime());
}

dtime_t I_ConvertTimeToMs(dtime_t value)
{
    return value / 1000000LL;
}

dtime_t I_ConvertTimeFromMs(dtime_t value)
{
    return value * 1000000LL;
}

//
// I_Sleep
//
// Sleeps for the specified number of nanoseconds, yielding control to the
// operating system. In actuality, the highest resolution availible with
// the select() function is 1 microsecond, but the nanosecond parameter
// is used for consistency with I_GetTime().
//
void I_Sleep(dtime_t sleep_time)
{
#if defined UNIX
    usleep(sleep_time / 1000LL);

#elif defined(WIN32)
    Sleep(sleep_time / 1000000LL);

#else
    SDL_Delay(sleep_time / 1000000LL);

#endif
}

//
// I_Yield
//
// Sleeps for 1 millisecond
//
void I_Yield()
{
    I_Sleep(1000LL * 1000LL); // sleep for 1ms
}

//
// I_WaitVBL
//
// I_WaitVBL is never used to actually synchronize to the
// vertical blank. Instead, it's used for delay purposes.
//
void I_WaitVBL(int32_t count)
{
    I_Sleep(1000000LL * 1000LL * count / 70);
}

EXTERN_CVAR(language)

void SetLanguageIDs()
{
    const char *langid = language.cstring();

    if (strcmp(langid, "auto") == 0)
    {
        // Just set the first preferred language. Is there a use case
        // for doing anything else right now? - Dasho
        SDL_Locale *pref_langs = SDL_GetPreferredLocales();
        if (pref_langs && pref_langs->language != NULL) // NULL language means end of array; country is optional
        {
            size_t length = strlen(pref_langs->language);
            if (length >= 3)
            {
                uint32_t lang  = MAKE_ID(tolower(pref_langs->language[0]), tolower(pref_langs->language[1]),
                                         tolower(pref_langs->language[2]), '\0');
                LanguageIDs[0] = lang;
                LanguageIDs[1] = lang;
                LanguageIDs[2] = lang;
                LanguageIDs[3] = lang;
            }
            else if (pref_langs->country != NULL)
            {
                uint32_t lang  = MAKE_ID(tolower(pref_langs->language[0]), tolower(pref_langs->language[1]),
                                         tolower(pref_langs->country[0]), '\0');
                LanguageIDs[0] = lang;
                LanguageIDs[1] = lang;
                LanguageIDs[2] = lang;
                LanguageIDs[3] = lang;
            }
            else
            {
                uint32_t lang = MAKE_ID(tolower(pref_langs->language[0]), tolower(pref_langs->language[1]), '\0', '\0');
                LanguageIDs[0] = lang;
                LanguageIDs[1] = lang;
                LanguageIDs[2] = lang;
                LanguageIDs[3] = lang;
            }
        }
        else // Default (English)
        {
            uint32_t lang  = MAKE_ID('*', '*', '\0', '\0');
            LanguageIDs[0] = lang;
            LanguageIDs[1] = lang;
            LanguageIDs[2] = lang;
            LanguageIDs[3] = lang;
        }
        if (pref_langs)
            SDL_free(pref_langs);
    }
    else
    {
        char slang[4] = {'\0', '\0', '\0', '\0'};
        strncpy(slang, langid, ARRAY_LENGTH(slang) - 1);
        uint32_t lang  = MAKE_ID(tolower(slang[0]), tolower(slang[1]), tolower(slang[2]), tolower(slang[3]));
        LanguageIDs[0] = lang;
        LanguageIDs[1] = lang;
        LanguageIDs[2] = lang;
        LanguageIDs[3] = lang;
    }
}

//
// I_Init
//
void I_Init(void)
{
    I_InitSound();
    I_InitHardware();
    UI_Initialize();
}

void I_FinishClockCalibration()
{
}

//
// I_Quit
//
static int32_t has_exited;

void I_Quit(void)
{
    has_exited = 1; /* Prevent infinitely recursive exits -- killough */

    G_ClearSnapshots();

    CL_QuitNetGame(NQ_SILENT);

    M_SaveDefaults();

    // I_ShutdownHardware();

    CloseNetwork();

    DConsoleAlias::DestroyAll();
}

//
// I_Error
//
bool gameisdead;

#define MAX_ERRORTEXT 8192

void STACK_ARGS call_terms(void);

#ifdef MUD_RELEASE
#define I_BREAK
#else
#if defined(_WIN32)
#if defined(__MINGW32__)
#define I_BREAK                                                                                                        \
    {                                                                                                                  \
        asm("int $0x03");                                                                                              \
    }
#elif defined(_MSC_VER)
#define I_BREAK                                                                                                        \
    {                                                                                                                  \
        __debugbreak();                                                                                                \
    }
#else
#define I_BREAK
#endif
#elif defined(__APPLE_CC__)
#define I_BREAK                                                                                                        \
    {                                                                                                                  \
        __builtin_trap();                                                                                              \
    }
#elif defined(UNIX)
#if defined __GNUC__
#define I_BREAK                                                                                                        \
    {                                                                                                                  \
        __builtin_trap();                                                                                              \
    }
#else
#define I_BREAK
#endif
#else
#define I_BREAK
#endif
#endif

void STACK_ARGS I_Error(const char *error, ...)
{
    char errortext[MAX_ERRORTEXT];
    char messagetext[MAX_ERRORTEXT];

    va_list argptr;
    va_start(argptr, error);
    int32_t index = vsnprintf(errortext, ARRAY_LENGTH(errortext), error, argptr);
    if (SDL_GetError()[0] != '\0')
    {
        snprintf(messagetext, ARRAY_LENGTH(messagetext), "%s\nLast SDL Error:\n%s\n", errortext, SDL_GetError());
        SDL_ClearError();
    }
    else
    {
        snprintf(messagetext, ARRAY_LENGTH(messagetext), "%s\n", errortext);
    }
    va_end(argptr);

    I_ErrorMessageBox(messagetext);

    throw CDoomError(messagetext);
}

void STACK_ARGS I_Warning(const char *warning, ...)
{
    va_list argptr;
    char    warningtext[MAX_ERRORTEXT];

    va_start(argptr, warning);
    vsprintf(warningtext, warning, argptr);
    va_end(argptr);

    Printf(PRINT_WARNING, "\n%s\n", warningtext);
}

void I_CallAssert(const char *file, int line, const char *message, ...)
{
    va_list argptr;
    char    asserttext[MAX_ERRORTEXT];
    va_start(argptr, message);
    vsprintf(asserttext, message, argptr);
    va_end(argptr);

    char infotext[MAX_ERRORTEXT];
    snprintf(infotext, MAX_ERRORTEXT, "%s\n\nfile: %s line: %i", asserttext, file, line);

    I_Error(infotext);
}

char DoomStartupTitle[256] = {0};

void I_SetTitleString(const char *title)
{
    int32_t i;

    for (i = 0; title[i]; i++)
        DoomStartupTitle[i] = title[i] | 0x80;
}

#ifdef X11
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#endif

//
// I_ConsoleInput
//
#ifdef _WIN32
std::string I_ConsoleInput(void)
{
    // denis - todo - implement this properly!!!
    /* denis - this probably won't work for a gui sdl app. if it does work, please uncomment!
    static char     text[1024] = {0};
    static char     buffer[1024] = {0};
    uint32_t    len = strlen(buffer);

    while(kbhit() && len < sizeof(text))
    {
        char ch = (char)getch();

        // input the character
        if(ch == '\b' && len)
        {
            buffer[--len] = 0;
            // john - backspace hack
            fwrite(&ch, 1, 1, stdout);
            ch = ' ';
            fwrite(&ch, 1, 1, stdout);
            ch = '\b';
        }
        else
            buffer[len++] = ch;
        buffer[len] = 0;

        // recalculate length
        len = strlen(buffer);

        // echo character back to user
        fwrite(&ch, 1, 1, stdout);
        fflush(stdout);
    }

    if(len && buffer[len - 1] == '\n' || buffer[len - 1] == '\r')
    {
        // echo newline back to user
        char ch = '\n';
        fwrite(&ch, 1, 1, stdout);
        fflush(stdout);

        strcpy(text, buffer);
        text[len-1] = 0; // rip off the /n and terminate
        buffer[0] = 0;
        len = 0;

        return text;
    }
*/
    return "";
}

#else

std::string I_ConsoleInput(void)
{
    std::string ret;
    static char text[1024] = {0};
    int32_t     len;

    fd_set fdr;
    FD_ZERO(&fdr);
    FD_SET(0, &fdr);
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 0;

    if (select(1, &fdr, NULL, NULL, &tv) <= 0)
        return "";

    len = read(0, text + strlen(text),
               sizeof(text) - strlen(text)); // denis - fixme - make it read until the next linebreak instead

    if (len < 1)
        return "";

    len = strlen(text);

    if (strlen(text) >= sizeof(text))
    {
        if (text[len - 1] == '\n' || text[len - 1] == '\r')
            text[len - 1] = 0; // rip off the /n and terminate

        ret = text;
        memset(text, 0, sizeof(text));
        return ret;
    }

    if (text[len - 1] == '\n' || text[len - 1] == '\r')
    {
        text[len - 1] = 0;

        ret = text;
        memset(text, 0, sizeof(text));
        return ret;
    }

    return "";
}
#endif

//
// I_IsHeadless
//
// Returns true if no application window will be created.
//
bool I_IsHeadless()
{
    static bool headless;
    static bool initialized = false;
    if (!initialized)
    {
        headless    = Args.CheckParm("-novideo") || Args.CheckParm("+demotest");
        initialized = true;
    }

    return headless;
}

const char *ODAMEX_ERROR_TITLE = "MUD " DOTVERSIONSTR " Fatal Error";

#if defined(SDL20)

void I_ErrorMessageBox(const char *message)
{
    bool debugger = false;

#if defined(MUD_DEBUG) && (defined(_MSC_VER) || defined(__MINGW32__))
    if (IsDebuggerPresent())
    {
        debugger = true;
    }
#endif

    if (!debugger)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, ODAMEX_ERROR_TITLE, message, NULL);
    }
    else
    {
        const SDL_MessageBoxButtonData buttons[] = {
            {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Ok"},
            {0, 0, "Debug"},
        };

        ISDL20Window *iwindow = (ISDL20Window *)I_GetWindow();

        const SDL_MessageBoxData messagebox_data = {
            SDL_MESSAGEBOX_ERROR,                        /* .flags */
            iwindow ? iwindow->getSDLWindow() : nullptr, /* .window */
            "MUD Error",                                 /* .title */
            message,                                     /* .message */
            SDL_arraysize(buttons),                      /* .numbuttons */
            buttons,                                     /* .buttons */
            nullptr                                      /* .colorScheme */
        };

        int buttonid;
        SDL_ShowMessageBox(&messagebox_data, &buttonid);

        if (buttonid == 0)
        {
            I_BREAK;
        }
    }
}

#elif defined(_MSCVER)

void I_ErrorMessageBox(const char *message)
{
    MessageBoxA(NULL, message, ODAMEX_ERROR_TITLE, MB_OK);
}

#elif OSX

void I_ErrorMessageBox(const char *message)
{
    CFStringRef macErrorMessage = CFStringCreateWithCString(NULL, message, kCFStringEncodingMacRoman);
    CFUserNotificationDisplayAlert(0, 0, NULL, NULL, NULL, CFSTR(ODAMEX_ERROR_TITLE), macErrorMessage, CFSTR("OK"),
                                   NULL, NULL, NULL);
    CFRelease(macErrorMessage);
}

#else

void I_ErrorMessageBox(const char *message)
{
    fprintf(stderr, "%s\n%s\n", ODAMEX_ERROR_TITLE, message);
}

#endif

#if defined(_DEBUG)

BEGIN_COMMAND(debug_userfilename)
{
    if (argc < 2)
    {
        Printf("debug_userfilename: needs a path to check.\n");
        return;
    }

    std::string userfile = M_GetUserFileName(argv[1]);
    Printf("Resolved to: %s\n", userfile.c_str());
}
END_COMMAND(debug_userfilename)

#endif

VERSION_CONTROL(i_system_cpp, "$Id: 918a04f32bf429bb5c3ef5cbabc737323f5dc92b $")
