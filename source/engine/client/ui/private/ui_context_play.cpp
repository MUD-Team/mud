
#include "ui_context_play.h"
#include <assert.h>

#include "i_system.h"
#include "ui_playerview.h"


UIContextPlay::UIContextPlay(Rml::Context *context)
{
    ElementPlayerView::InitializeContext(context);
}

UIContextPlay::~UIContextPlay()
{
}
