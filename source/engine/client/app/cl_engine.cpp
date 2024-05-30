
#include <Poco/Activity.h>
#include <Poco/Util/Application.h>
#include <Poco/Util/Subsystem.h>
#include <SDL.h>

#include <stack>

#include "cl_main.h"
#include "d_main.h"
#include "i_input.h"
#include "i_sound.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "mud_includes.h"

using Poco::Activity;
using Poco::Util::Application;
using Poco::Util::Subsystem;

// clean me up
DArgs Args;

class MUDEngine : public Subsystem
{
  public:
    MUDEngine()
    {
    }

    const char *name() const
    {
        return "MUDEngine";
    }

  protected:
    void initialize(Application &app)
    {
        // [AM] Set crash callbacks, so we get something useful from crashes.
#ifdef NDEBUG
        I_SetCrashCallbacks();
#endif

#if defined(UNIX)
        if (!getuid() || !geteuid())
            I_Error("root user detected, quitting odamex immediately");
#endif

        std::vector<const char *> cstrings;
        cstrings.reserve(app.argv().size());

        std::transform(app.argv().begin(), app.argv().end(), std::back_inserter(cstrings),
                       [](const auto &string) { return string.c_str(); });

        ::Args.SetArgs(cstrings.size(), (char **)cstrings.data());

        if (PHYSFS_init(::Args.GetArg(0)) == 0)
            I_Error("Could not initialize PHYSFS:\n%d\n", PHYSFS_getLastErrorCode());

        PHYSFS_setWriteDir(M_GetWriteDir().c_str());
        // Ensure certain directories exist in the write folder
        // These should be no-ops if already present - Dasho
        PHYSFS_mkdir("assets");
        // Ensure downloads folder exists (I don't think we need to do this for core in the writeable folder)
        PHYSFS_mkdir("assets/downloads");
        PHYSFS_mkdir("saves");
        PHYSFS_mkdir("screenshots");
        PHYSFS_mkdir("soundfonts");

        PHYSFS_mount(M_GetBinaryDir().c_str(), NULL, 0);
        PHYSFS_mount(M_GetWriteDir().c_str(), NULL, 0);

        PHYSFS_mount(M_GetBinaryDir().append("assets").append(PATHSEP).append("core").c_str(), NULL, 0);
        PHYSFS_mount(
            M_GetBinaryDir().append("assets").append(PATHSEP).append("core").append(PATHSEP).append("common").c_str(),
            NULL, 0);
        PHYSFS_mount(
            M_GetBinaryDir().append("assets").append(PATHSEP).append("core").append(PATHSEP).append("client").c_str(),
            NULL, 0);
        PHYSFS_mount(M_GetWriteDir().append("assets").append(PATHSEP).append("downloads").c_str(), NULL, 0);

        // TODO: configurable with -game and root config json
        PHYSFS_mount(M_GetBinaryDir().append("assets").append(PATHSEP).append("example").c_str(), NULL, 0);

        /*
                if (::Args.CheckParm("--version"))
                {
        #ifdef _WIN32
                    PHYSFS_File *fh = PHYSFS_openWrite("mud-version.txt");
                    if (!fh)
                        exit(EXIT_FAILURE);

                    std::string str;
                    StrFormat(str, "MUD %s\n", NiceVersion());
                    const int32_t ok = PHYSFS_writeBytes(fh, str.data(), str.size());
                    if (ok != str.size())
                    {
                        PHYSFS_deinit();
                        exit(EXIT_FAILURE);
                    }

                    PHYSFS_close(fh);
                    PHYSFS_deinit();
        #else
                    printf("MUD Client %s\n", NiceVersion());
        #endif
                    exit(EXIT_SUCCESS);
                }
        */

        const char *CON_FILE = ::Args.CheckValue("-confile");
        if (CON_FILE)
        {
            CON.open(CON_FILE, std::ios::in);
        }

        /*
        // denis - if argv[1] starts with "mud://"
        if (argc == 2 && argv && argv[1])
        {
            const char *protocol = "mud://";
            const char *uri      = argv[1];

            if (strncmp(uri, protocol, strlen(protocol)) == 0)
            {
                std::string location = uri + strlen(protocol);
                size_t      term     = location.find_first_of('/');

                if (term == std::string::npos)
                    term = location.length();

                Args.AppendArg("-connect");
                Args.AppendArg(location.substr(0, term).c_str());
            }
        }
        */

        uint32_t sdl_flags = SDL_INIT_TIMER;

#ifdef _MSC_VER
        // [SL] per the SDL documentation, SDL's parachute, used to cleanup
        // after a crash, causes the MSVC debugger to be unusable
        sdl_flags |= SDL_INIT_NOPARACHUTE;
#endif

        if (SDL_Init(sdl_flags) == -1)
            I_Error("Could not initialize SDL:\n%s\n", SDL_GetError());

        // todo: this is more initialization
        D_DoomMain();

        mInitialized = true;
    }

    void uninitialize()
    {
        if (!mInitialized)
        {
            return;
        }

        D_DoomMainShutdown();

        I_ShutdownSound();
        I_ShutdownInput();
        DObject::StaticShutdown();

        I_Quit();
        SDL_Quit();
        PHYSFS_deinit();
    }

    bool mInitialized = false;

};

void CL_Engine_Init()
{
    Application::instance().addSubsystem(new MUDEngine());
}