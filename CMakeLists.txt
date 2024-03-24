cmake_minimum_required(VERSION 3.15)

if ("${CMAKE_TOOLCHAIN_FILE}" MATCHES "vcpkg.cmake")
    include ("vcpkg_triplets/vcpkg-triplets.cmake")
endif()

project(MUD VERSION 0.1)

find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_mixer_ext CONFIG REQUIRED)

if (MSVC)
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
endif()

add_subdirectory(source)
