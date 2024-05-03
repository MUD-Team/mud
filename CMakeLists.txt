cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if ("${CMAKE_TOOLCHAIN_FILE}" MATCHES "vcpkg.cmake")
    if (CMAKE_HOST_UNIX)
        include ("vcpkg_triplets/vcpkg-triplets-linux.cmake")
    elseif ("${CMAKE_GENERATOR}" MATCHES "MinGW Makefiles") # for some reason the regular (MINGW) CMake check fails here - Dasho
        include ("vcpkg_triplets/vcpkg-triplets-mingw.cmake")
    else()
        include ("vcpkg_triplets/vcpkg-triplets-windows.cmake")
    endif()
endif()

project(MUD VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(CURL REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(fluidlite CONFIG REQUIRED)
find_package(Freetype REQUIRED)
find_package(Lua REQUIRED)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin )

option(MUD_SANITIZE "Enable code sanitizing" OFF)
option(MUD_PROFILING "Enable Profiling" OFF)

# Profiling
if (MUD_PROFILING)
  add_definitions(-DMUD_PROFILING -DTRACY_ENABLE -DTRACY_CALLSTACK)
endif()

if (WIN32 AND NOT MINGW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")    
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
    
    if (NOT CLANG)
        # get the number of logical cores for parallel build
        cmake_host_system_information(RESULT LOGICAL_CORES QUERY NUMBER_OF_LOGICAL_CORES)
        math(EXPR COMPILE_CORES "${LOGICAL_CORES} - 1")  
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP${COMPILE_CORES}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP${COMPILE_CORES}")
    endif()

    # Disable some very noisy warnings from the MSVC build
    # CRT security and POSIX deprecation warnings
    add_definitions("-D_CRT_SECURE_NO_WARNINGS /wd4996")
    # Loss of precision/data on assignment, requires lots of explicit casting
    add_definitions("/wd4244 /wd4267")
    # Unreferenced formal parameter, and there are many of these
    add_definitions("/wd4100")
elseif(NOT MINGW)

    add_definitions(-DUNIX)
    
    if (MUD_SANITIZE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address -static-libasan")
    endif()
    
endif()

add_subdirectory(source)
