//----------------------------------------------------------------------------
//  EDGE Console Interface code.
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2024 The EDGE Team.
//  Copyright (c) 1998       Randy Heit
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
// Originally based on the ZDoom console code, by Randy Heit
// (rheit@iastate.edu).  Randy Heit has given his permission to
// release this code under the GPL, for which the EDGE Team is very
// grateful.

#include <stdarg.h>
#include <string.h>

#include <vector>

#include "con_main.h"
#include "con_var.h"
#include "ddf_language.h"
#include "dm_state.h"
#include "e_input.h"
#include "e_player.h"
#include "edge_profiling.h"
#include "epi.h"
#include "epi_str_compare.h"
#include "epi_str_util.h"
#include "g_game.h"
#include "i_defs_gl.h"
#include "i_system.h"
#include "m_argv.h"
#include "r_draw.h"
#include "r_image.h"
#include "r_modes.h"
#include "w_files.h"

static constexpr uint8_t kConsoleWipeTics = 12;

EDGE_DEFINE_CONSOLE_VARIABLE(debug_fps, "0", kConsoleVariableFlagArchive)
EDGE_DEFINE_CONSOLE_VARIABLE(debug_position, "0", kConsoleVariableFlagArchive)

static ConsoleVisibility console_visible;

// stores the console toggle effect
static int   console_wipe_active   = 0;
static int   console_wipe_position = 0;

static RGBAColor current_color;

extern void StartupProgressMessage(const char *message);

extern ConsoleVariable pixel_aspect_ratio;

static constexpr uint8_t kMaximumConsoleLines = 160;

// entry [0] is the bottom-most one
static ConsoleLine *console_lines[kMaximumConsoleLines];
static int          console_used_lines        = 0;
static bool         console_partial_last_line = false;

// the console row that is displayed at the bottom of screen, -1 if cmdline
// is the bottom one.
static int bottom_row = -1;

static constexpr uint8_t kMaximumConsoleInput = 255;

static char input_line[kMaximumConsoleInput + 2];
static int  input_position = 0;

int                    console_cursor;

static constexpr uint8_t kConsoleKeyRepeatDelay = ((250 * kTicRate) / 1000);
static constexpr uint8_t kConsoleKeyRepeatRate  = (kTicRate / 15);

// HISTORY

static constexpr uint8_t kConsoleMaximumCommandHistory = 100;

static std::string *cmd_history[kConsoleMaximumCommandHistory];

static int command_used_history = 0;

// when browsing the cmdhistory, this shows the current index. Otherwise it's
// -1.
static int command_history_position = -1;

// always type kInputEventKeyDown
static int repeat_key;
static int repeat_countdown;

// tells whether shift is pressed, and pgup/dn should scroll to top/bottom of
// linebuffer.
static bool keys_shifted;

static bool tabbed_last;

static int scroll_direction;

static void ConsoleAddLine(const char *s, bool partial)
{
    if (console_partial_last_line)
    {
        EPI_ASSERT(console_lines[0]);

        console_lines[0]->Append(s);

        console_partial_last_line = partial;
        return;
    }

    // scroll everything up

    delete console_lines[kMaximumConsoleLines - 1];

    for (int i = kMaximumConsoleLines - 1; i > 0; i--)
        console_lines[i] = console_lines[i - 1];

    RGBAColor col = current_color;

    if (col == SG_GRAY_RGBA32 && (epi::StringPrefixCaseCompareASCII(s, "WARNING") == 0))
        col = SG_DARK_ORANGE_RGBA32;

    console_lines[0] = new ConsoleLine(s, col);

    console_partial_last_line = partial;

    if (console_used_lines < kMaximumConsoleLines)
        console_used_lines++;
}

static void ConsoleAddCmdHistory(const char *s)
{
    // don't add if same as previous command
    if (command_used_history > 0)
        if (strcmp(s, cmd_history[0]->c_str()) == 0)
            return;

    // scroll everything up
    delete cmd_history[kConsoleMaximumCommandHistory - 1];

    for (int i = kConsoleMaximumCommandHistory - 1; i > 0; i--)
        cmd_history[i] = cmd_history[i - 1];

    cmd_history[0] = new std::string(s);

    if (command_used_history < kConsoleMaximumCommandHistory)
        command_used_history++;
}

static void ConsoleClearInputLine(void)
{
    input_line[0]  = 0;
    input_position = 0;
}

void SetConsoleVisible(ConsoleVisibility v)
{
    if (v == kConsoleVisibilityToggle)
    {
        v = (console_visible == kConsoleVisibilityNotVisible) ? kConsoleVisibilityMaximal
                                                              : kConsoleVisibilityNotVisible;

        scroll_direction = 0;
    }

    if (console_visible == v)
        return;

    console_visible = v;

    if (v == kConsoleVisibilityMaximal)
    {
        tabbed_last = false;
    }

    if (!console_wipe_active)
    {
        console_wipe_active   = true;
        console_wipe_position = (v == kConsoleVisibilityMaximal) ? 0 : kConsoleWipeTics;
    }

    if (console_visible != kConsoleVisibilityNotVisible)
    {
        GrabCursor(false);
    }
}

static void StripWhitespace(char *src)
{
    const char *start = src;

    while (*start && epi::IsSpaceASCII(*start))
        start++;

    const char *end = src + strlen(src);

    while (end > start && epi::IsSpaceASCII(end[-1]))
        end--;

    while (start < end)
    {
        *src++ = *start++;
    }

    *src = 0;
}

static void SplitIntoLines(char *src)
{
    char *dest = src;
    char *line = dest;

    while (*src)
    {
        if (*src == '\n')
        {
            *dest++ = 0;

            ConsoleAddLine(line, false);

            line = dest;

            src++;
            continue;
        }

        *dest++ = *src++;
    }

    *dest++ = 0;

    if (line[0])
    {
        ConsoleAddLine(line, true);
    }

    current_color = SG_GRAY_RGBA32;
}

void ConsolePrint(const char *message, ...)
{
    va_list argptr;
    char    buffer[1024];

    va_start(argptr, message);
    vsprintf(buffer, message, argptr);
    va_end(argptr);

    SplitIntoLines(buffer);
}

void ConsoleMessage(const char *message, ...)
{
    va_list argptr;
    char    buffer[1024];

    va_start(argptr, message);

    // Print the message into a text string
    vsprintf(buffer, message, argptr);

    va_end(argptr);    

    strcat(buffer, "\n");

    SplitIntoLines(buffer);
}

void ConsoleMessageLDF(const char *lookup, ...)
{
    va_list argptr;
    char    buffer[1024];

    lookup = language[lookup];

    va_start(argptr, lookup);
    vsprintf(buffer, lookup, argptr);
    va_end(argptr);

    strcat(buffer, "\n");

    SplitIntoLines(buffer);
}

void ImportantConsoleMessageLDF(const char *lookup, ...)
{
    va_list argptr;
    char    buffer[1024];

    lookup = language[lookup];

    va_start(argptr, lookup);
    vsprintf(buffer, lookup, argptr);
    va_end(argptr);

    strcat(buffer, "\n");

    SplitIntoLines(buffer);
}

void ConsoleMessageColor(RGBAColor col)
{
    current_color = col;
}

static int   XMUL = 0;
static int   FNSZ;
static float FNSZ_ratio;

static void CalcSizes()
{
    if (current_screen_width < 1024)
        FNSZ = 16;
    else
        FNSZ = 24;
}


static void GotoEndOfLine(void)
{
    if (command_history_position < 0)
        input_position = strlen(input_line);
    else
        input_position = strlen(cmd_history[command_history_position]->c_str());

    console_cursor = 0;
}

static void EditHistory(void)
{
    if (command_history_position >= 0)
    {
        strcpy(input_line, cmd_history[command_history_position]->c_str());

        command_history_position = -1;
    }
}

static void InsertChar(char ch)
{
    // make room for new character, shift the trailing NUL too

    for (int j = kMaximumConsoleInput - 2; j >= input_position; j--)
        input_line[j + 1] = input_line[j];

    input_line[kMaximumConsoleInput - 1] = 0;

    input_line[input_position++] = ch;
}

static char KeyToCharacter(int key, bool shift, bool ctrl)
{
    if (ctrl)
        return 0;

    if (key < 32 || key > 126)
        return 0;

    if (!shift)
        return (char)key;

    // the following assumes a US keyboard layout
    switch (key)
    {
    case '1':
        return '!';
    case '2':
        return '@';
    case '3':
        return '#';
    case '4':
        return '$';
    case '5':
        return '%';
    case '6':
        return '^';
    case '7':
        return '&';
    case '8':
        return '*';
    case '9':
        return '(';
    case '0':
        return ')';

    case '`':
        return '~';
    case '-':
        return '_';
    case '=':
        return '+';
    case '\\':
        return '|';
    case '[':
        return '{';
    case ']':
        return '}';
    case ';':
        return ':';
    case '\'':
        return '"';
    case ',':
        return '<';
    case '.':
        return '>';
    case '/':
        return '?';
    case '@':
        return '\'';
    }

    return epi::ToUpperASCII(key);
}

static void ListCompletions(std::vector<const char *> &list, int word_len, int max_row, RGBAColor color)
{
    int max_col = current_screen_width / XMUL - 4;
    max_col     = glm_clamp(24, max_col, 78);

    char buffer[200];
    int  buf_len = 0;

    buffer[buf_len] = 0;

    char temp[200];
    char last_ja = 0;

    for (int i = 0; i < (int)list.size(); i++)
    {
        const char *name  = list[i];
        int         n_len = (int)strlen(name);

        // support for names with a '.' in them
        const char *dotpos = strchr(name, '.');
        if (dotpos && dotpos > name + word_len)
        {
            if (last_ja == dotpos[-1])
                continue;

            last_ja = dotpos[-1];

            n_len = (int)(dotpos - name);

            strcpy(temp, name);
            temp[n_len] = 0;

            name = temp;
        }
        else
            last_ja = 0;

        if (n_len >= max_col * 2 / 3)
        {
            ConsoleMessageColor(color);
            ConsolePrint("  %s\n", name);
            max_row--;
            continue;
        }

        if (buf_len + 1 + n_len > max_col)
        {
            ConsoleMessageColor(color);
            ConsolePrint("  %s\n", buffer);
            max_row--;

            buf_len         = 0;
            buffer[buf_len] = 0;

            if (max_row <= 0)
            {
                ConsoleMessageColor(color);
                ConsolePrint("  etc...\n");
                break;
            }
        }

        if (buf_len > 0)
            buffer[buf_len++] = ' ';

        strcpy(buffer + buf_len, name);

        buf_len += n_len;
    }

    if (buf_len > 0)
    {
        ConsoleMessageColor(color);
        ConsolePrint("  %s\n", buffer);
    }
}

static void TabComplete(void)
{
    EditHistory();

    // check if we are positioned after a word
    {
        if (input_position == 0)
            return;

        if (epi::IsDigitASCII(input_line[0]))
            return;

        for (int i = 0; i < input_position; i++)
        {
            char ch = input_line[i];

            if (!(epi::IsAlphanumericASCII(ch) || ch == '_' || ch == '.'))
                return;
        }
    }

    char save_ch               = input_line[input_position];
    input_line[input_position] = 0;

    std::vector<const char *> match_cmds;
    std::vector<const char *> match_vars;
    std::vector<const char *> match_keys;

    int num_cmd = MatchConsoleCommands(match_cmds, input_line);
    int num_var = MatchConsoleVariables(match_vars, input_line);
    int num_key = 0; ///  E_MatchAllKeys(match_keys, input_line);

    // we have an unambiguous match, no need to print anything
    if (num_cmd + num_var + num_key == 1)
    {
        input_line[input_position] = save_ch;

        const char *name = (num_var > 0) ? match_vars[0] : (num_key > 0) ? match_keys[0] : match_cmds[0];

        EPI_ASSERT((int)strlen(name) >= input_position);

        for (name += input_position; *name; name++)
            InsertChar(*name);

        if (save_ch != ' ')
            InsertChar(' ');

        console_cursor = 0;
        return;
    }

    // show what we were trying to match
    ConsoleMessageColor(SG_LIGHT_BLUE_RGBA32);
    ConsolePrint(">%s\n", input_line);

    input_line[input_position] = save_ch;

    if (num_cmd + num_var + num_key == 0)
    {
        ConsolePrint("No matches.\n");
        return;
    }

    if (match_vars.size() > 0)
    {
        ConsolePrint("%u Possible variables:\n", (int)match_vars.size());

        ListCompletions(match_vars, input_position, 7, SG_SPRING_GREEN_RGBA32);
    }

    if (match_keys.size() > 0)
    {
        ConsolePrint("%u Possible keys:\n", (int)match_keys.size());

        ListCompletions(match_keys, input_position, 4, SG_SPRING_GREEN_RGBA32);
    }

    if (match_cmds.size() > 0)
    {
        ConsolePrint("%u Possible commands:\n", (int)match_cmds.size());

        ListCompletions(match_cmds, input_position, 3, SG_SPRING_GREEN_RGBA32);
    }

    // Add as many common characters as possible
    // (e.g. "mou <TAB>" should add the s, e and _).

    // begin by lumping all completions into one list
    unsigned int i;

    for (i = 0; i < match_keys.size(); i++)
        match_vars.push_back(match_keys[i]);

    for (i = 0; i < match_cmds.size(); i++)
        match_vars.push_back(match_cmds[i]);

    int pos = input_position;

    for (;;)
    {
        char ch = match_vars[0][pos];
        if (!ch)
            return;

        for (i = 1; i < match_vars.size(); i++)
            if (match_vars[i][pos] != ch)
                return;

        InsertChar(ch);

        pos++;
    }
}

void ConsoleHandleKey(int key, bool shift, bool ctrl)
{
    switch (key)
    {
    case kRightAlt:
    case kRightControl:
        // Do nothing
        break;

    case kRightShift:
        // SHIFT was pressed
        keys_shifted = true;
        break;

    case kPageUp:
        if (shift)
            // Move to top of console buffer
            bottom_row = GLM_MAX(-1, console_used_lines - 10);
        else
            // Start scrolling console buffer up
            scroll_direction = +1;
        break;

    case kPageDown:
        if (shift)
            // Move to bottom of console buffer
            bottom_row = -1;
        else
            // Start scrolling console buffer down
            scroll_direction = -1;
        break;

    case kMouseWheelUp:
        bottom_row += 4;
        if (bottom_row > GLM_MAX(-1, console_used_lines - 10))
            bottom_row = GLM_MAX(-1, console_used_lines - 10);
        break;

    case kMouseWheelDown:
        bottom_row -= 4;
        if (bottom_row < -1)
            bottom_row = -1;
        break;

    case kHome:
        // Move cursor to start of line
        input_position = 0;
        console_cursor = 0;
        break;

    case kEnd:
        // Move cursor to end of line
        GotoEndOfLine();
        break;

    case kUpArrow:
        // Move to previous entry in the command history
        if (command_history_position < command_used_history - 1)
        {
            command_history_position++;
            GotoEndOfLine();
        }
        tabbed_last = false;
        break;

    case kDownArrow:
        // Move to next entry in the command history
        if (command_history_position > -1)
        {
            command_history_position--;
            GotoEndOfLine();
        }
        tabbed_last = false;
        break;

    case kLeftArrow:
        // Move cursor left one character
        if (input_position > 0)
            input_position--;

        console_cursor = 0;
        break;

    case kRightArrow:
        // Move cursor right one character
        if (command_history_position < 0)
        {
            if (input_line[input_position] != 0)
                input_position++;
        }
        else
        {
            if (cmd_history[command_history_position]->c_str()[input_position] != 0)
                input_position++;
        }
        console_cursor = 0;
        break;

    case kEnter:
        EditHistory();

        // Execute command line (ENTER)
        StripWhitespace(input_line);

        if (strlen(input_line) == 0)
        {
            ConsoleMessageColor(SG_LIGHT_BLUE_RGBA32);
            ConsolePrint(">\n");
        }
        else
        {
            // Add it to history & draw it
            ConsoleAddCmdHistory(input_line);

            ConsoleMessageColor(SG_LIGHT_BLUE_RGBA32);
            ConsolePrint(">%s\n", input_line);

            // Run it!
            TryConsoleCommand(input_line);
        }

        ConsoleClearInputLine();

        // Bring user back to current line after entering command
        bottom_row -= kMaximumConsoleLines;
        if (bottom_row < -1)
            bottom_row = -1;

        tabbed_last = false;
        break;

    case kBackspace:
        // Erase character to left of cursor
        EditHistory();

        if (input_position > 0)
        {
            input_position--;

            // shift characters back
            for (int j = input_position; j < kMaximumConsoleInput - 2; j++)
                input_line[j] = input_line[j + 1];
        }

        tabbed_last    = false;
        console_cursor = 0;
        break;

    case kDelete:
        // Erase charater under cursor
        EditHistory();

        if (input_line[input_position] != 0)
        {
            // shift characters back
            for (int j = input_position; j < kMaximumConsoleInput - 2; j++)
                input_line[j] = input_line[j + 1];
        }

        tabbed_last    = false;
        console_cursor = 0;
        break;

    case kTab:
        // Try to do tab-completion
        TabComplete();
        break;

    case kEscape:
        // Close console, clear command line, but if we're in the
        // fullscreen console mode, there's nothing to fall back on
        // if it's closed.
        ConsoleClearInputLine();

        command_history_position = -1;
        tabbed_last              = false;

        SetConsoleVisible(kConsoleVisibilityNotVisible);
        break;

    // Allow screenshotting of console too
    case kFunction1:
    case kPrintScreen:
        DeferredScreenShot();
        break;

    default: {
        char ch = KeyToCharacter(key, shift, ctrl);

        // ignore non-printable characters
        if (ch == 0)
            break;

        // no room?
        if (input_position >= kMaximumConsoleInput - 1)
            break;

        EditHistory();
        InsertChar(ch);

        tabbed_last    = false;
        console_cursor = 0;
    }
    break;
    }
}

static int GetKeycode(InputEvent *ev)
{
    int sym = ev->value.key.sym;

    switch (sym)
    {
    case kTab:
    case kPageUp:
    case kPageDown:
    case kHome:
    case kEnd:
    case kLeftArrow:
    case kRightArrow:
    case kBackspace:
    case kDelete:
    case kUpArrow:
    case kDownArrow:
    case kMouseWheelUp:
    case kMouseWheelDown:
    case kEnter:
    case kEscape:
    case kRightShift:
    case kFunction1:
    case kPrintScreen:
        return sym;

    default:
        break;
    }

    if (epi::IsPrintASCII(sym))
        return sym;

    return -1;
}

bool ConsoleResponder(InputEvent *ev)
{
    if (ev->type != kInputEventKeyUp && ev->type != kInputEventKeyDown)
        return false;

    if (ev->type == kInputEventKeyDown && CheckKeyMatch(key_console, ev->value.key.sym))
    {
        ClearEventInput();
        SetConsoleVisible(kConsoleVisibilityToggle);
        return true;
    }

    if (console_visible == kConsoleVisibilityNotVisible)
        return false;

    int key = GetKeycode(ev);
    if (key < 0)
        return true;

    if (ev->type == kInputEventKeyUp)
    {
        if (key == repeat_key)
            repeat_countdown = 0;

        switch (key)
        {
        case kPageUp:
        case kPageDown:
            scroll_direction = 0;
            break;
        case kRightShift:
            keys_shifted = false;
            break;
        default:
            break;
        }
    }
    else
    {
        // Okay, fine. Most keys don't repeat
        switch (key)
        {
        case kRightArrow:
        case kLeftArrow:
        case kUpArrow:
        case kDownArrow:
        case kSpace:
        case kBackspace:
        case kDelete:
            repeat_countdown = kConsoleKeyRepeatDelay;
            break;
        default:
            repeat_countdown = 0;
            break;
        }

        repeat_key = key;

        ConsoleHandleKey(key, keys_shifted, false);
    }

    return true; // eat all keyboard events
}

void ConsoleTicker(void)
{
    console_cursor = (console_cursor + 1) & 31;

    if (console_visible != kConsoleVisibilityNotVisible)
    {
        // Handle repeating keys
        switch (scroll_direction)
        {
        case +1:
            if (bottom_row < kMaximumConsoleLines - 10)
                bottom_row++;
            break;

        case -1:
            if (bottom_row > -1)
                bottom_row--;
            break;

        default:
            if (repeat_countdown)
            {
                repeat_countdown -= 1;

                while (repeat_countdown <= 0)
                {
                    repeat_countdown += kConsoleKeyRepeatRate;
                    ConsoleHandleKey(repeat_key, keys_shifted, false);
                }
            }
            break;
        }
    }

    if (console_wipe_active)
    {
        if (console_visible == kConsoleVisibilityNotVisible)
        {
            console_wipe_position--;
            if (console_wipe_position <= 0)
                console_wipe_active = false;
        }
        else
        {
            console_wipe_position++;
            if (console_wipe_position >= kConsoleWipeTics)
                console_wipe_active = false;
        }
    }
}

//
// Initialises the console
//
void ConsoleInit(void)
{
    SortConsoleVariables();

    console_used_lines   = 0;
    command_used_history = 0;

    bottom_row               = -1;
    command_history_position = -1;

    ConsoleClearInputLine();

    current_color = SG_GRAY_RGBA32;

    ConsoleAddLine("", false);
    ConsoleAddLine("", false);
}

void ConsoleStart(void)
{
    working_directory = home_directory;
    console_visible   = kConsoleVisibilityNotVisible;
    console_cursor    = 0;
    StartupProgressMessage("Starting console...");
}

void ClearConsoleLines()
{
    for (int i = 0; i < console_used_lines; i++)
    {
        console_lines[i]->Clear();
    }
    console_used_lines = 0;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
