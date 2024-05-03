
#pragma once

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>

#include "../../sdl/i_sdl.h"

class UIInput
{
  public:
    UIInput();

  private:
    friend class UI;

    void postEvent(SDL_Event &ev);
    void processEvents();

    std::vector<SDL_Event> mSDLEvents;
};