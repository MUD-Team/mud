#include "ui_mud_plugin.h"

#include <RmlUi/Debugger.h>
#include <RmlUi/Core/Context.h>
#include <assert.h>

#include "i_system.h"
#include "ui_console.h"


MUDPlugin::MUDPlugin()
{
}

int32_t MUDPlugin::GetEventClasses()
{
    return EVT_BASIC | EVT_DOCUMENT;
}

void MUDPlugin::OnInitialise()
{
    mPlayerViewInstancer = Rml::MakeUnique<Rml::ElementInstancerGeneric<ElementPlayerView>>();
    Rml::Factory::RegisterElementInstancer("playerview", mPlayerViewInstancer.get());

    mConsoleInstancer = Rml::MakeUnique<Rml::ElementInstancerGeneric<ElementConsole>>();
    Rml::Factory::RegisterElementInstancer("console", mConsoleInstancer.get());
}

void MUDPlugin::OnShutdown()
{
    mConsoleInstancer.reset();
    mPlayerViewInstancer.reset();
}

void MUDPlugin::OnContextCreate(Rml::Context *context)
{
    if (context->GetName() == "play")
    {
        assert(!mPlayContext.get());
        mPlayContext = Rml::MakeUnique<UIContextPlay>(context);

        // todo: move this to the ui debug console command, we're currently using fonts from here
        Rml::Debugger::Initialise(context);
        Rml::Debugger::SetVisible(false);
    }
}