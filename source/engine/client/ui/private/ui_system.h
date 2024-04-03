#pragma once

#include <RmlUi/Core/SystemInterface.h>

class UISystemInterface : public Rml::SystemInterface
{
  public:
    UISystemInterface();
    virtual ~UISystemInterface();

    // -- Inherited from Rml::SystemInterface  --

    double GetElapsedTime() override;

    void SetMouseCursor(const Rml::String &cursor_name) override;

    void SetClipboardText(const Rml::String &text) override;
    void GetClipboardText(Rml::String &text) override;

  private:
};
