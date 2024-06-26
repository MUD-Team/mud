// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
//
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
//  ClientOptions module.
//
//-----------------------------------------------------------------------------

#include <LuaBridge.h>
#include <c_dispatch.h>
#include <cmdlib.h>

#include "c_cvars.h"
#include "i_video.h"
#include "lua_client_private.h"
#include "v_video.h"

EXTERN_CVAR(cl_mouselook)
EXTERN_CVAR(snd_musicvolume)
EXTERN_CVAR(snd_sfxvolume)

class LuaOptions
{
  public:
    static void open(lua_State *L)
    {
        luabridge::getGlobalNamespace(L)
            .beginNamespace("mud")
            .beginNamespace("options")
            .addFunction("get_input_options",
                         [L] {
                             luabridge::LuaRef options = luabridge::newTable(L);
                             options["mouselook"]      = !!cl_mouselook;
                             options["mouselook_help"] = cl_mouselook.helptext();
                             return options;
                         })
            .addFunction("set_input_options", [L](luabridge::LuaRef options) { cl_mouselook = !!options["mouselook"]; })

            .addFunction("get_display_options",
                         [L] {
                             luabridge::LuaRef options = luabridge::newTable(L);

                             // display modes
                             luabridge::LuaRef modes = luabridge::newTable(L);
                             options["current_mode"] = buildDisplayModes(L, modes);
                             options["modes"]        = modes;

                             return options;
                         })
            .addFunction("set_display_options", [L](luabridge::LuaRef options) { applyDisplayOptions(L, options); })

            .addFunction("get_sound_options",
                         [L] {
                             luabridge::LuaRef options = luabridge::newTable(L);
                             options["music_volume"]   = snd_musicvolume.value();
                             options["sfx_volume"]     = snd_sfxvolume.value();
                             return options;
                         })
            .addFunction("set_sound_options",
                         [L](luabridge::LuaRef options) {
                             float musicvolume = options["music_volume"];
                             float sfxvolume   = options["sfx_volume"];

                             snd_musicvolume.Set(musicvolume);
                             snd_sfxvolume.Set(sfxvolume);
                         })
            .endNamespace()
            .endNamespace();
    }

    static void applyDisplayOptions(lua_State *L, luabridge::LuaRef &options)
    {
        std::string       current_mode = options["current_mode"];
        luabridge::LuaRef modes        = options["modes"];
        luabridge::LuaRef mode         = modes[std::stoi(current_mode)];
        int32_t           width        = mode["width"];
        int32_t           height       = mode["height"];

        int32_t hiwidth  = I_GetVideoWidth();
        int32_t hiheight = I_GetVideoHeight();

        if (width != hiwidth || height != hiheight)
        {
            std::string command;
            StrFormat(command, "vid_setmode %i %i", width, height);
            AddCommandString(command);

            V_ForceVideoModeAdjustment();
        }
    }

    static int32_t buildDisplayModes(lua_State *L, luabridge::LuaRef &modes)
    {
        int32_t hiwidth  = I_GetVideoWidth();
        int32_t hiheight = I_GetVideoHeight();

        // gathers a list of unique resolutions availible for the current
        // screen mode (windowed or fullscreen)
        bool fullscreen = I_GetWindow()->getVideoMode().isFullScreen();

        typedef std::vector<std::pair<uint16_t, uint16_t>> MenuModeList;
        MenuModeList                                       menumodelist;

        const IVideoModeList *videomodelist = I_GetVideoCapabilities()->getSupportedVideoModes();
        for (IVideoModeList::const_iterator it = videomodelist->begin(); it != videomodelist->end(); ++it)
            if (it->isFullScreen() == fullscreen)
                menumodelist.push_back(std::make_pair(it->width, it->height));
        menumodelist.erase(std::unique(menumodelist.begin(), menumodelist.end()), menumodelist.end());

        MenuModeList::const_iterator mode_it = menumodelist.begin();

        int32_t index        = 0;
        int32_t current_mode = -1;
        for (auto mode : menumodelist)
        {
            int32_t width  = mode.first;
            int32_t height = mode.second;

            luabridge::LuaRef vmode = luabridge::newTable(L);

            if (width == hiwidth && height == hiheight)
            {
                current_mode = index;
            }

            vmode["id"]     = index;
            vmode["width"]  = width;
            vmode["height"] = height;

            modes[index++] = vmode;
        }

        if (current_mode == -1)
        {
            luabridge::LuaRef vmode = luabridge::newTable(L);

            vmode["id"]     = index;
            vmode["width"]  = hiwidth;
            vmode["height"] = hiheight;
            current_mode    = index;
            modes[index++]  = vmode;
        }

        return current_mode;
    }
};

void LUA_OpenClientOptions(lua_State *L)
{
    LuaOptions::open(L);
}