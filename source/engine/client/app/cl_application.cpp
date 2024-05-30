
#include <Poco/Glob.h>
#include <Poco/Util/Application.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>

#include "cl_main.h"
#include "d_main.h"

using Poco::Util::Application;
using Poco::Util::HelpFormatter;
using Poco::Util::Option;
using Poco::Util::OptionCallback;
using Poco::Util::OptionSet;

void CL_Engine_Init();

class MUDClientApp : public Application
{
  public:
    MUDClientApp()
    {
        setUnixOptions(true);
    }

  protected:
    void initialize(Application &self)
    {
        try
        {
            CL_Engine_Init();
            Application::initialize(self);
        }
        catch (CDoomError &error)
        {
            exit(-1);
        }
    }

    void defineOptions(OptionSet &options)
    {
        // workaround until move options processing here, currently errors for uknown options
        stopOptionsProcessing();
        /*
          Application::defineOptions(options);

          options.addOption(Option("help", "h", "display argument help information")
                                .required(false)
                                .repeatable(false)
                                .callback(OptionCallback<MUDClientApp>(this, &MUDClientApp::handleHelp)));
        */
    }

    void handleHelp(const std::string &name, const std::string &value)
    {
        HelpFormatter helpFormatter(options());
        helpFormatter.setCommand(commandName());
        helpFormatter.setUsage("OPTIONS");
        helpFormatter.setHeader("MUD Client");
        helpFormatter.format(std::cout);
        stopOptionsProcessing();
        mHelpRequested = true;
    }

    int main(const std::vector<std::string> &args)
    {
        if (mHelpRequested)
        {
            return Application::EXIT_OK;
        }

        while (!CL_QuitRequested())
        {
            try
            {
                D_RunTics(CL_RunTics, CL_DisplayTics);
            }
            catch (CDoomError &error)
            {
                break;
            }
        }

        return Application::EXIT_OK;
    }

  private:
    bool mHelpRequested = false;
};

POCO_APP_MAIN(MUDClientApp)