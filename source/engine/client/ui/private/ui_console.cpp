
#include "ui_console.h"

#include <RmlUi/Core/Elements/ElementFormControlTextArea.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Input.h>
#include <assert.h>

#include "c_dispatch.h"
#include "doomtype.h"

static ElementConsole *sElementConsole = nullptr;

std::vector<ElementConsole::ConsoleLine> ElementConsole::mConsoleLines;
bool                                     ElementConsole::mDirty = true;

// ============================================================================
//
// UIConsoleHistory class interface
//
// Stores a copy of each line of text entered on the command line and provides
// iteration functions to recall previous command lines entered.
//
// ============================================================================

class UIConsoleHistory
{
  public:
    UIConsoleHistory();

    void clear();

    void resetPosition();

    void               addString(const std::string &str);
    const std::string &getString() const;

    void movePositionUp();
    void movePositionDown();

    void dump();

  private:
    static const size_t MAX_HISTORY_ITEMS = 50;

    typedef std::list<std::string> UIConsoleHistoryList;
    UIConsoleHistoryList           history;

    UIConsoleHistoryList::const_iterator history_it;
};

// ============================================================================
//
// UIConsoleHistory class implementation
//
// ============================================================================

UIConsoleHistory::UIConsoleHistory()
{
    resetPosition();
}

void UIConsoleHistory::clear()
{
    history.clear();
    resetPosition();
}

void UIConsoleHistory::resetPosition()
{
    history_it = history.end();
}

void UIConsoleHistory::addString(const std::string &str)
{
    // only add the string if it's different from the most recent in history
    if (!str.empty() && (history.empty() || str.compare(history.back()) != 0))
    {
        while (history.size() >= MAX_HISTORY_ITEMS)
            history.pop_front();
        history.push_back(str);
    }
}

const std::string &UIConsoleHistory::getString() const
{
    if (history_it == history.end())
    {
        static std::string blank_string;
        return blank_string;
    }

    return *history_it;
}

void UIConsoleHistory::movePositionUp()
{
    if (history_it != history.begin())
        --history_it;
}

void UIConsoleHistory::movePositionDown()
{
    if (history_it != history.end())
        ++history_it;
}

void UIConsoleHistory::dump()
{
    for (UIConsoleHistoryList::const_iterator it = history.begin(); it != history.end(); ++it)
        Printf(PRINT_HIGH, "   %s\n", it->c_str());
}

static UIConsoleHistory sHistory;

ElementConsole::ElementConsole(const Rml::String &tag) : Rml::Element(tag)
{
    assert(!sElementConsole);

    sElementConsole = this;

    mConsoleTypes[CONSOLE_ERROR].mLevel         = CONSOLE_ERROR;
    mConsoleTypes[CONSOLE_ERROR].mClassName     = "error";
    mConsoleTypes[CONSOLE_ERROR].mAlertContents = "!";
    mConsoleTypes[CONSOLE_ERROR].mButtonName    = "error_button";

    mConsoleTypes[CONSOLE_WARNING].mLevel         = CONSOLE_WARNING;
    mConsoleTypes[CONSOLE_WARNING].mClassName     = "warning";
    mConsoleTypes[CONSOLE_WARNING].mAlertContents = "!";
    mConsoleTypes[CONSOLE_WARNING].mButtonName    = "warning_button";

    mConsoleTypes[CONSOLE_INFO].mLevel         = CONSOLE_INFO;
    mConsoleTypes[CONSOLE_INFO].mClassName     = "info";
    mConsoleTypes[CONSOLE_INFO].mAlertContents = "i";
    mConsoleTypes[CONSOLE_INFO].mButtonName    = "info_button";
}

ElementConsole::~ElementConsole()
{
    sElementConsole = nullptr;

    if (mConsoleInput)
    {
        mConsoleInput->RemoveEventListener(Rml::EventId::Change, this);
        mConsoleInput->RemoveEventListener(Rml::EventId::Keyup, this);
    }

    if (mConsoleContent)
    {
        mConsoleContent->RemoveEventListener(Rml::EventId::Resize, this);
    }
}

void ElementConsole::OnUpdate()
{
    Element::OnUpdate();

    if (mDirty)
    {
        if (mConsoleContent)
        {
            Rml::String messages;
            for (auto &line : mConsoleLines)
            {
                messages += Rml::CreateString(
                    128, "<div class=\"log-entry\"><div class=\"icon %s\">%s</div><p class=\"message\">",
                    mConsoleTypes[line.mLevel].mClassName.c_str(), mConsoleTypes[line.mLevel].mAlertContents.c_str());
                messages += Rml::StringUtilities::EncodeRml(line.mLineSource);
                messages += "</p></div>";
            }

            mConsoleContent->SetInnerRML(messages);
        }

        mDirty = false;
    }
}

void ElementConsole::OnLayout()
{
    Element::OnLayout();

    if (mConsoleContent && mConsoleContent->HasChildNodes())
    {
        mConsoleContent->GetLastChild()->ScrollIntoView();
    }
}
void ElementConsole::OnChildAdd(Rml::Element *child)
{
    Element::OnChildAdd(child);
    if (child->GetId() == "content")
    {
        mConsoleContent = child;
        mConsoleContent->AddEventListener(Rml::EventId::Resize, this);
    }

    if (child->GetId() == "console-input")
    {
        mConsoleInput = child;
        mConsoleInput->AddEventListener(Rml::EventId::Change, this);
        mConsoleInput->AddEventListener(Rml::EventId::Keyup, this);
    }
}

void ElementConsole::OnChildRemove(Rml::Element *child)
{
    if (child->GetId() == "content")
    {
        if (mConsoleContent)
        {
            mConsoleContent->RemoveEventListener(Rml::EventId::Resize, this);
            mConsoleContent = nullptr;
        }
    }

    if (child->GetId() == "console-input")
    {
        if (mConsoleInput)
        {
            mConsoleInput->RemoveEventListener(Rml::EventId::Change, this);
            mConsoleInput->RemoveEventListener(Rml::EventId::Keyup, this);
            mConsoleInput = nullptr;
        }
    }

    Element::OnChildRemove(child);
}

void ElementConsole::ProcessEvent(Rml::Event &event)
{
    if (event.GetTargetElement() == mConsoleInput)
    {
        Rml::ElementFormControlTextArea *textarea = (Rml::ElementFormControlTextArea *)mConsoleInput;

        if (event == Rml::EventId::Keyup)
        {
            Rml::Input::KeyIdentifier key_identifier =
                (Rml::Input::KeyIdentifier)event.GetParameter<int32_t>("key_identifier", 0);
            if (key_identifier == Rml::Input::KI_TAB)
            {
            }
            else if (key_identifier == Rml::Input::KI_OEM_3) // tilde
            {
                textarea->SetValue("");
            }
            else if (key_identifier == Rml::Input::KI_UP)
            {
                sHistory.movePositionUp();
                const std::string &value = sHistory.getString();
                if (value.size())
                {
                    textarea->SetValue(value);
                    textarea->SetSelectionRange(value.size(), value.size());
                }
                event.StopPropagation();
            }
            else if (key_identifier == Rml::Input::KI_DOWN)
            {
                sHistory.movePositionDown();
                const std::string &value = sHistory.getString();
                if (value.size())
                {
                    textarea->SetValue(value);
                    textarea->SetSelectionRange(value.size(), value.size());
                }
                event.StopPropagation();
            }
            else if (key_identifier == Rml::Input::KI_RETURN)
            {
                Rml::String value = textarea->GetValue();
                if (value.size())
                {
                    AddCommandString(value);
                    sHistory.addString(value);
                    sHistory.resetPosition();
                    textarea->SetValue("");
                }
                event.StopPropagation();
            }
        }
    }

    if (event.GetTargetElement() == mConsoleContent)
    {
        if (event == Rml::EventId::Resize)
        {
            if (mConsoleContent->HasChildNodes())
                mConsoleContent->GetLastChild()->ScrollIntoView();
        }
    }
}

void ElementConsole::AddMessage(ConsoleLevel level, const std::string &message)
{
    ConsoleLine line;
    line.mLevel      = level;
    line.mLineSource = message;

    std::replace(line.mLineSource.begin(), line.mLineSource.end(), '\x1d', '=');
    std::replace(line.mLineSource.begin(), line.mLineSource.end(), '\x1e', '=');
    std::replace(line.mLineSource.begin(), line.mLineSource.end(), '\x1f', '=');
    auto result = std::remove(line.mLineSource.begin(), line.mLineSource.end(), '\n');
    line.mLineSource.erase(result, line.mLineSource.end());

    if (!line.mLineSource.size())
    {
        return;
    }

    mConsoleLines.emplace_back(line);

    mDirty = true;
}

void UI_PrintString(printlevel_t printlevel, const std::string &sanitized_str)
{
    ConsoleLevel level = ConsoleLevel::CONSOLE_MAX;

    switch (printlevel)
    {
    case PRINT_HIGH:
        level = ConsoleLevel::CONSOLE_INFO;
        break;
    case PRINT_ERROR:
        level = ConsoleLevel::CONSOLE_ERROR;
        break;
    case PRINT_WARNING:
        level = ConsoleLevel::CONSOLE_WARNING;
        break;
    }

    if (level == ConsoleLevel::CONSOLE_MAX)
    {
        return;
    }

    ElementConsole::AddMessage(level, sanitized_str);
}