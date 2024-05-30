
add_subdirectory(minilzo)
add_subdirectory(physfs)

add_subdirectory(luabridge)
add_subdirectory(imgui)
add_subdirectory(rmlui)

add_subdirectory(libimagequant)

if (MUD_PROFILING)
    add_subdirectory(tracy)
endif()

