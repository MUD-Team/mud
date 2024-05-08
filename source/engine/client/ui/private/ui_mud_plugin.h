#pragma once

#include <RmlUi/Core/Plugin.h>
#include <RmlUi/Core/ElementInstancer.h>

#include "ui_playerview.h"
#include "ui_context_play.h"

class ElementConsole;

class MUDPlugin : public Rml::Plugin {
public:
	MUDPlugin();

private:
	int32_t GetEventClasses() override;

	void OnInitialise() override;

	void OnShutdown() override;

	void OnContextCreate(Rml::Context* context) override;


	// Main Context
	//void InitializeMainContext(Rml::Context* context);
	//bool LoadConsoleElement();
    
	Rml::UniquePtr<Rml::Context> mMainContext;

    Rml::UniquePtr<UIContextPlay> mPlayContext;

	// Player View 
	Rml::UniquePtr<Rml::ElementInstancerGeneric<ElementPlayerView>> mPlayerViewInstancer;

	// Player View 
	Rml::UniquePtr<Rml::ElementInstancerGeneric<ElementConsole>> mConsoleInstancer;
};
