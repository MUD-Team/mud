

# #define SERVER_APP for use in the source code
add_definitions(-DSERVER_APP)
add_definitions(-DODAMEX_NO_GITVER)

# Server
file(GLOB SERVER_SOURCES *.cpp *.h)

get_target_property(COMMON_SOURCES engine-common INTERFACE_SOURCES)

include_directories(./ ../common ../../libraries/physfs ../../libraries/stb)

add_executable(mud-server ${COMMON_SOURCES} ${SERVER_SOURCES})

set_target_properties(mud-server PROPERTIES DEBUG_POSTFIX "_DEBUG")
set_target_properties(mud-server PROPERTIES RELWITHDEBINFO_POSTFIX "_RELWITHDEBINFO")

set (MUD_SERVER_LIBRARIES mudproto minilzo physfs-static JsonCpp::JsonCpp)

if (MUD_PROFILING)
  set (MUD_SERVER_LIBRARIES ${MUD_SERVER_LIBRARIES} tracy)
endif()

if (WIN32)
    set (MUD_SERVER_LIBRARIES ${MUD_SERVER_LIBRARIES} winmm wsock32 shlwapi)
endif()

target_link_libraries(mud-server PRIVATE ${MUD_SERVER_LIBRARIES})

