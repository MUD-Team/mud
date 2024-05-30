
# Enable default SIMD settings for standard builds.
option(USE_DEFAULT_SIMD "Use default SIMD optimization settings" ON)

# #define CLIENT_APP for use in the source code
add_definitions(-DCLIENT_APP)
add_definitions(-DODAMEX_NO_GITVER)

file(GLOB CLIENT_SOURCES *.cpp *.h)
file(GLOB CLIENT_SDL_SOURCES sdl/*.cpp sdl/*.h)
file(GLOB CLIENT_UI_SOURCES ui/private/*.cpp)
file(GLOB CLIENT_SCRIPT_SOURCES script/private/*.cpp)
file(GLOB CLIENT_APP_SOURCES app/*.cpp)

get_target_property(COMMON_SOURCES engine-common INTERFACE_SOURCES)

# Client includes
include_directories(./ ./sdl ./ui ../common ../../libraries/physfs ../../libraries/stb)

add_executable(mud-client WIN32 ${CLIENT_SOURCES} ${CLIENT_UI_SOURCES} ${CLIENT_SCRIPT_SOURCES} ${CLIENT_SDL_SOURCES} ${COMMON_SOURCES} ${CLIENT_APP_SOURCES})
set_target_properties(mud-client PROPERTIES DEBUG_POSTFIX "_DEBUG")
set_target_properties(mud-client PROPERTIES RELWITHDEBINFO_POSTFIX "_RELWITHDEBINFO")

if(MSVC)
  set_target_properties(mud-client PROPERTIES LINK_FLAGS "/ENTRY:wmainCRTStartup")
endif()


set (MUD_CLIENT_LIBRARIES imgui libimagequant minilzo physfs-static lua luabridge)

if (MUD_PROFILING)
  set (MUD_CLIENT_LIBRARIES ${MUD_CLIENT_LIBRARIES} tracy)
endif()

if (WIN32)
    set (MUD_CLIENT_LIBRARIES "${MUD_CLIENT_LIBRARIES}" Shlwapi)
endif()


target_link_libraries(mud-client PRIVATE mudproto ${MUD_CLIENT_LIBRARIES} RmlCore RmlDebugger RmlLua Poco::Util Poco::Foundation CURL::libcurl protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite
                      $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
                      $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
                      $<IF:$<TARGET_EXISTS:fluidlite::fluidlite>,fluidlite::fluidlite,fluidlite::fluidlite-static>)