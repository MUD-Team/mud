

# #define SERVER_APP for use in the source code
add_definitions(-DSERVER_APP)
add_definitions(-DODAMEX_NO_GITVER)

# Server
file(GLOB SERVER_SOURCES *.cpp *.h)

get_target_property(COMMON_SOURCES engine-common INTERFACE_SOURCES)

include_directories(./ ../common)

add_executable(mud-server ${COMMON_SOURCES} ${SERVER_SOURCES})

target_link_libraries(mud-server mudproto ZLIB::ZLIB JsonCpp::JsonCpp)

if(WIN32)
  target_link_libraries(mud-server winmm wsock32 shlwapi)
endif()
