#pragma once

#include "../sdl/i_sdl.h"

class IRenderSurface;
class ISDL20Window;
class PixelFormat;

bool UI_Initialize();
void UI_Shutdown();

bool UI_RenderInitialized();

void UI_SetMode(uint16_t width, uint16_t height, const PixelFormat *format, ISDL20Window *window, bool vsync,
                const char *render_scale_quality = NULL);

void UI_PostEvent(SDL_Event &ev);

void UI_LoadCore();
