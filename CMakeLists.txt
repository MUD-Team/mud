cmake_minimum_required(VERSION 3.15)

if ("${CMAKE_TOOLCHAIN_FILE}" MATCHES "vcpkg.cmake")
    if (CMAKE_HOST_UNIX)
        include ("vcpkg_triplets/vcpkg-triplets-linux.cmake")
    else()
        include ("vcpkg_triplets/vcpkg-triplets-windows.cmake")
    endif()
endif()

project(MUD VERSION 0.1)

find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(fluidlite CONFIG REQUIRED)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin )

option(EDGE_SANITIZE "Enable code sanitizing" OFF)

if (WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
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
else()

    add_definitions(-DUNIX)
    
    if (EDGE_SANITIZE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address -static-libasan")
    endif()
    
endif()

add_subdirectory(source)
