
#include <RmlUi/Core/Core.h>
#include <SDL.h>

#include "backend/imgui_impl_sdl2.h"
#include "backend/imgui_impl_sdlrenderer2.h"
#include "c_cvars.h"
#include "sdl/i_video_sdl20.h"
#include "ui_render.h"

EXTERN_CVAR(r_imgui)

class IMGUI
{

  public:
    IMGUI()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // IF using Docking Branch
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

        UIRenderInterface *interface = (UIRenderInterface *)Rml::GetRenderInterface();
        ImGui_ImplSDL2_InitForSDLRenderer(interface->getSDLWindow()->getSDLWindow(), interface->getSDLRenderer());
        ImGui_ImplSDLRenderer2_Init(interface->getSDLRenderer());
    }

    ~IMGUI()
    {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    void beginFrame()
    {
        if (!mShown)
        {
            return;
        }

        mInFrame = true;

        ImGui_ImplSDL2_NewFrame();
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();

        if (ImGui::Button("Hide ImGui"))
        {
            r_imgui.Set("0");
        }
    }

    void endFrame()
    {
        if (!mShown && !mInFrame)
        {
            return;
        }

        ImGui::Render();

        UIRenderInterface *interface = (UIRenderInterface *)Rml::GetRenderInterface();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), interface->getSDLRenderer());

        mInFrame = false;
    }

    bool handleInput(const std::vector<SDL_Event> &events)
    {
        if (!mShown)
        {
            return false;
        }

        for (const SDL_Event &event : events)
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
        }

        return true;
    }

    void show(bool shown)
    {
        mShown = shown;
    }

  private:
    bool mInFrame = false;
    bool mShown   = false;
};

static std::unique_ptr<IMGUI> g_imgui;

void UI_IMGUI_Init()
{
    if (g_imgui.get())
    {
        return;
    }

    g_imgui = std::make_unique<IMGUI>();
}

void UI_IMGUI_BeginFrame()
{
    if (!g_imgui.get())
    {
        return;
    }

    g_imgui->beginFrame();
}

bool UI_IMGUI_HandleInput(const std::vector<SDL_Event> &events)
{

    if (!g_imgui.get())
    {
        return false;
    }

    return g_imgui->handleInput(events);
}

void UI_IMGUI_EndFrame()
{
    if (!g_imgui.get())
    {
        return;
    }

    g_imgui->endFrame();
}

void UI_IMGUI_Shutdown()
{
    g_imgui.reset();
}

CVAR_FUNC_IMPL(r_imgui)
{
    if (!g_imgui.get())
    {
        return;
    }

    g_imgui->show(!!var);
}
