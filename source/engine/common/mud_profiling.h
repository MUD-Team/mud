
#pragma once

#ifdef __cplusplus

#ifdef MUD_PROFILING
	
	#include <tracy/Tracy.hpp>

	#define MUD_ZoneNamed(varname, active) ZoneNamed(varname, active)
	#define MUD_ZoneNamedN(varname, name, active) ZoneNamedN(varname, name, active)
	#define MUD_ZoneNamedC(varname, color, active) ZoneNamedC(varname, color, active)
	#define MUD_ZoneNamedNC(varname, name, color, active) ZoneNamedNC(varname, name, color, active)

	#define MUD_ZoneScoped ZoneScoped
	#define MUD_ZoneScopedN(name) ZoneScopedN(name)
	#define MUD_ZoneScopedC(color) ZoneScopedC(color)
	#define MUD_ZoneScopedNC(name, color) ZoneScopedNC(name, color)

	#define MUD_ZoneText(txt, size) ZoneText(txt, size)
	#define MUD_ZoneName(txt, size) ZoneName(txt, size)

	#define MUD_TracyPlot(name, val) TracyPlot(name, val)

	#define MUD_FrameMark FrameMark
	#define MUD_FrameMarkNamed(name) FrameMarkNamed(name)
	#define MUD_FrameMarkStart(name) FrameMarkStart(name)
	#define MUD_FrameMarkEnd(name) FrameMarkEnd(name)

#else

	#define MUD_ZoneNamed(varname, active)
	#define MUD_ZoneNamedN(varname, name, active)
	#define MUD_ZoneNamedC(varname, color, active)
	#define MUD_ZoneNamedNC(varname, name, color, active)

	#define MUD_ZoneScoped
	#define MUD_ZoneScopedN(name)
	#define MUD_ZoneScopedC(color)
	#define MUD_ZoneScopedNC(name, color)

	#define MUD_ZoneText(txt, size)
	#define MUD_ZoneName(txt, size)

	#define MUD_TracyPlot(name, val)

	#define MUD_FrameMark
	#define MUD_FrameMarkNamed(name)
	#define MUD_FrameMarkStart(name)
	#define MUD_FrameMarkEnd(name)

#endif

#else
	#include <tracy/TracyC.h>
#endif

