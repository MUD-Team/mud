
#include "ui_system.h"

#include <RmlUi/Core.h>
#include <SDL.h>

UISystemInterface::UISystemInterface()
{
    Rml::SetSystemInterface(this);
}

UISystemInterface::~UISystemInterface()
{
}

double UISystemInterface::GetElapsedTime()
{
    static const Uint64 start     = SDL_GetPerformanceCounter();
    static const double frequency = double(SDL_GetPerformanceFrequency());
    return double(SDL_GetPerformanceCounter() - start) / frequency;
}

void UISystemInterface::SetMouseCursor(const Rml::String &cursor_name)
{
}

void UISystemInterface::SetClipboardText(const Rml::String &text_utf8)
{
    SDL_SetClipboardText(text_utf8.c_str());
}

void UISystemInterface::GetClipboardText(Rml::String &text)
{
    char *raw_text = SDL_GetClipboardText();
    text           = Rml::String(raw_text);
    SDL_free(raw_text);
}
