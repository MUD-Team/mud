# CMake 3.13 needed for -S and -B params in library compilation.
#
# Note that if you are running Linux, there are many ways to get newer
# versions of CMake.
# 
# - Kitware offers binary downloads direct from their website, which you can
#   extract to /usr/local or ~/.local.
# - Ubuntu LTS users can install the CMake snap, and Kitware also runs an
#   official Ubuntu CMake apt repository.
# - Debian users can get a new version through backports.
# - CentOS users can get a new version through EPEL.
# - If you have Python installed, you can install CMake through pip.
#

cmake_minimum_required(VERSION 3.13)

project(Odamex VERSION 10.4.0)

# CMAKE_INSTALL_BINDIR and CMAKE_INSTALL_DATADIR will be changed if GNUInstallDirs is availible
set(CMAKE_INSTALL_BINDIR "bin")
set(CMAKE_INSTALL_DATADIR "share")
include(GNUInstallDirs OPTIONAL)

add_definitions(-DINSTALL_BINDIR="${CMAKE_INSTALL_BINDIR}")
add_definitions(-DINSTALL_DATADIR="${CMAKE_INSTALL_DATADIR}")

# options
option(BUILD_CLIENT "Build client target" 1)
option(BUILD_SERVER "Build server target" 1)
option(BUILD_MASTER "Build master server target" 1)
option(BUILD_OR_FAIL "Must build the BUILD_* targets or else generation will fail" 0)
option(USE_INTERNAL_DEUTEX "Use internal DeuTex" 1)
option(USE_LTO "Build Release builds with Link Time Optimization" 1)

set(PROJECT_COPYRIGHT "2006-2023")
set(PROJECT_RC_VERSION "10,4,0,0")
set(PROJECT_COMPANY "The Odamex Team")

# Ensure that we can use folders in projects.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# identify the target CPU
# adapted from the FindJNI.cmake module included with the CMake distribution
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(ODAMEX_TARGET_ARCH "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i[3-9]86$")
  set(ODAMEX_TARGET_ARCH "i386")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^alpha")
  set(ODAMEX_TARGET_ARCH "alpha")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
  set(ODAMEX_TARGET_ARCH "arm")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
  set(ODAMEX_TARGET_ARCH "ppc64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)")
  set(ODAMEX_TARGET_ARCH "ppc")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^sparc")
  # Both flavors can run on the same processor
  set(ODAMEX_TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}" "sparc" "sparcv9")
else()
  set(ODAMEX_TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()

list(REMOVE_DUPLICATES ODAMEX_TARGET_ARCH)
message(STATUS "Target architecture: ${ODAMEX_TARGET_ARCH}")

# Default build type
if(NOT CMAKE_CONFIGURATION_TYPES)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
  endif()
  message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
  set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif()

if(NOT CMAKE_EXPORT_COMPILE_COMMANDS)
  # Export compile commands unless we're generating a modern project.
  if(CMAKE_GENERATOR MATCHES "Make" OR CMAKE_GENERATOR MATCHES "Ninja")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  else()
    set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)
  endif()
endif()
message(STATUS "Export Compile Commands: ${CMAKE_EXPORT_COMPILE_COMMANDS}")
set(CMAKE_EXPORT_COMPILE_COMMANDS "${CMAKE_EXPORT_COMPILE_COMMANDS}" CACHE BOOL
  "Export compile commands for use in supported editors."
  FORCE)

# Global compile options as shown in a GUI.
if(NOT MSVC)
  set(USE_COLOR_DIAGNOSTICS ON CACHE BOOL
    "Force the use of color diagnostics, necessary to get color output with Ninja.")
  set(USE_STATIC_STDLIB OFF CACHE BOOL
    "Statically link against the C and C++ Standard Library.")
  set(USE_SANITIZE_ADDRESS OFF CACHE BOOL
    "Turn on Address Sanitizer in Debug builds, requires GCC >= 4.8 or Clang >= 3.1")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES SunOS )
  set(SOLARIS 1)
endif()

if(USE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT HAS_LTO OUTPUT HAS_LTO_ERROR)
  if(HAS_LTO)
    message(STATUS "Link Time Optimization: ON")
  else()
    message(STATUS "Link Time Optimization: OFF")
  endif()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# git describe
#
# Grabs the git hash and branch a few different ways.
include(GetGitBranch)
include(GetGitRevisionDescription)
include(GetGitRevisionNumber)
get_git_head_revision(HEAD GIT_HASH)
git_describe(GIT_DESCRIBE --all --long --abbrev=4)
git_rev_count(HEAD GIT_REV_COUNT)
git_branch(HEAD GIT_BRANCH)
string(REGEX REPLACE "^(heads\/|tags\/)(.+)(-0-g)([0-9a-f]+)"
  "\\4" GIT_SHORT_HASH "${GIT_DESCRIBE}")

# Libraries

### zlib ###

if(BUILD_CLIENT OR BUILD_SERVER)
  find_package(ZLIB REQUIRED)
endif()

### libpng (dep: zlib) ###

if(BUILD_CLIENT)
  find_package(PNG)
endif()

### libcurl ###

# if(BUILD_CLIENT)
#  find_package(CURL REQUIRED)
#  if(TARGET CURL::libcurl)
#    set_target_properties(CURL::libcurl PROPERTIES IMPORTED_GLOBAL True)
#    if(WIN32)
#      target_link_libraries(CURL::libcurl INTERFACE ws2_32 crypt32)
#    endif()
#  endif()
# endif()


### JsonCpp ###

if(BUILD_SERVER)
  find_package(jsoncpp CONFIG REQUIRED)
endif()

### SDL libraries ###

if(BUILD_CLIENT)

  ### SDL2 ###

  if(WIN32)
    if(MSVC)
      file(DOWNLOAD
        "https://www.libsdl.org/release/SDL2-devel-2.0.20-VC.zip"
        "${CMAKE_CURRENT_BINARY_DIR}/SDL2-VC.zip"
        EXPECTED_HASH SHA256=5b1512ca6c9d2427bd2147da01e5e954241f8231df12f54a7074dccde416df18)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
        "${CMAKE_CURRENT_BINARY_DIR}/SDL2-VC.zip"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

      set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.20")
      set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include" CACHE PATH "")
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_LIBRARY "${SDL2_DIR}/lib/x64/SDL2.lib" CACHE FILEPATH "")
        set(SDL2MAIN_LIBRARY "${SDL2_DIR}/lib/x64/SDL2main.lib" CACHE FILEPATH "")
      else()
        set(SDL2_LIBRARY "${SDL2_DIR}/lib/x86/SDL2.lib" CACHE FILEPATH "")
        set(SDL2MAIN_LIBRARY "${SDL2_DIR}/lib/x86/SDL2main.lib" CACHE FILEPATH "")
      endif()
    else()
      file(DOWNLOAD
        "https://www.libsdl.org/release/SDL2-devel-2.0.20-mingw.tar.gz"
        "${CMAKE_CURRENT_BINARY_DIR}/SDL2-mingw.tar.gz"
        EXPECTED_HASH SHA256=38094d82a857d6c62352e5c5cdec74948c5b4d25c59cbd298d6d233568976bd1)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
        "${CMAKE_CURRENT_BINARY_DIR}/SDL2-mingw.tar.gz"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.20/x86_64-w64-mingw32")
      else()
        set(SDL2_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2-2.0.20/i686-w64-mingw32")
      endif()
      set(SDL2_INCLUDE_DIR "${SDL2_DIR}/include/SDL2" CACHE PATH "")
      set(SDL2_LIBRARY "${SDL2_DIR}/lib/libSDL2.dll.a" CACHE FILEPATH "")
      set(SDL2MAIN_LIBRARY "${SDL2_DIR}/lib/libSDL2main.a" CACHE FILEPATH "")
    endif()
  endif()

  find_package(SDL2)

  if(SDL2_FOUND)
    # SDL2 target.
    add_library(SDL2::SDL2 UNKNOWN IMPORTED GLOBAL)
    set_target_properties(SDL2::SDL2 PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
      IMPORTED_LOCATION "${SDL2_LIBRARY}")

    if(SDL2MAIN_LIBRARY)
      # SDL2main target.
      if(MINGW)
        # Gross hack to get mingw32 first in the linker order.
        add_library(SDL2::_SDL2main_detail UNKNOWN IMPORTED GLOBAL)
        set_target_properties(SDL2::_SDL2main_detail PROPERTIES
          IMPORTED_LOCATION "${SDL2MAIN_LIBRARY}")

        # Ensure that SDL2main comes before SDL2 in the linker order.  CMake
        # isn't smart enough to keep proper ordering for indirect dependencies
        # so we have to spell it out here.
        target_link_libraries(SDL2::_SDL2main_detail INTERFACE SDL2::SDL2)

        add_library(SDL2::SDL2main INTERFACE IMPORTED GLOBAL)
        set_target_properties(SDL2::SDL2main PROPERTIES
          IMPORTED_LIBNAME mingw32)
        target_link_libraries(SDL2::SDL2main INTERFACE SDL2::_SDL2main_detail)
      else()
        add_library(SDL2::SDL2main UNKNOWN IMPORTED GLOBAL)
        set_target_properties(SDL2::SDL2main PROPERTIES
          IMPORTED_LOCATION "${SDL2MAIN_LIBRARY}")
      endif()
    endif()
  endif()

  ### SDL2_mixer ###

  if(WIN32)
    if(MSVC)
      file(DOWNLOAD
        "https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-VC.zip"
        "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-VC.zip"
        EXPECTED_HASH SHA256=258788438b7e0c8abb386de01d1d77efe79287d9967ec92fbb3f89175120f0b0)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
        "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-VC.zip"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

      set(SDL2_MIXER_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-2.0.4")
      set(SDL2_MIXER_INCLUDE_DIR "${SDL2_MIXER_DIR}/include" CACHE PATH "")
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_MIXER_LIBRARY "${SDL2_MIXER_DIR}/lib/x64/SDL2_mixer.lib" CACHE FILEPATH "")
      else()
        set(SDL2_MIXER_LIBRARY "${SDL2_MIXER_DIR}/lib/x86/SDL2_mixer.lib" CACHE FILEPATH "")
      endif()
    else()
      file(DOWNLOAD
        "https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-mingw.tar.gz"
        "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-mingw.tar.gz"
        EXPECTED_HASH SHA256=14250b2ade20866c7b17cf1a5a5e2c6f3920c443fa3744f45658c8af405c09f1)
      execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xf
        "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-mingw.tar.gz"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_MIXER_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-2.0.4/x86_64-w64-mingw32")
      else()
        set(SDL2_MIXER_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer-2.0.4/i686-w64-mingw32")
      endif()
      set(SDL2_MIXER_INCLUDE_DIR "${SDL2_MIXER_DIR}/include/SDL2" CACHE PATH "")
      set(SDL2_MIXER_LIBRARY "${SDL2_MIXER_DIR}/lib/libSDL2_mixer.dll.a" CACHE FILEPATH "")
    endif()
  endif()

  find_package(SDL2_mixer)

  if(SDL2_MIXER_FOUND)
    # SDL2_mixer target.
    add_library(SDL2::mixer UNKNOWN IMPORTED GLOBAL)
    set_target_properties(SDL2::mixer PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SDL2_MIXER_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES SDL2::SDL2
      IMPORTED_LOCATION "${SDL2_MIXER_LIBRARY}")
  endif()
endif()

# WAD building
add_subdirectory(wad)

# Subdirectories for Odamex projects
if(BUILD_CLIENT OR BUILD_SERVER)
  add_subdirectory(common)
  add_subdirectory(odaproto)
endif()
if(BUILD_CLIENT)
  add_subdirectory(client)
endif()
if(BUILD_SERVER)
  add_subdirectory(server)
endif()
if(BUILD_MASTER)
  add_subdirectory(master)
endif()
if(NOT BUILD_CLIENT AND NOT BUILD_SERVER AND NOT BUILD_MASTER)
  message(FATAL_ERROR "No target chosen, doing nothing.")
endif()

# Packaging options.
# TODO: Integrate OSX stuff into here.
if(NOT APPLE)
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
  set(CPACK_PACKAGE_INSTALL_DIRECTORY Odamex)
  set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)

  set(CPACK_COMPONENTS_ALL client server common)
  set(CPACK_COMPONENT_CLIENT_DEPENDS common)
  set(CPACK_COMPONENT_CLIENT_DISPLAY_NAME "Odamex")
  set(CPACK_COMPONENT_SERVER_DEPENDS common)
  set(CPACK_COMPONENT_SERVER_DISPLAY_NAME "Odamex Dedicated Server")
  set(CPACK_COMPONENT_COMMON_DISPLAY_NAME "Support files")

  file(GLOB CONFIG_SAMPLES config-samples/*.cfg)
  if(WIN32)
    install(FILES LICENSE README
      DESTINATION .
      COMPONENT common)
    install(FILES ${CONFIG_SAMPLES}
      DESTINATION config-samples
      COMPONENT common)

    # Windows ZIP packages are "tarbombs" by default.
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
  else()
    install(FILES LICENSE README
      DESTINATION ${CMAKE_INSTALL_DATADIR}/odamex
      COMPONENT common)
    install(FILES ${CONFIG_SAMPLES}
      DESTINATION ${CMAKE_INSTALL_DATADIR}/odamex/config-samples
      COMPONENT common)

    option(ODAMEX_COMPONENT_PACKAGES "Create several rpm/deb packages for repository maintainers." OFF)
    if(ODAMEX_COMPONENT_PACKAGES)
      set(CPACK_RPM_COMPONENT_INSTALL YES)
      # TODO: RPM Dependencies

      set(CPACK_DEB_COMPONENT_INSTALL YES)
      # TODO: DEB Dependencies
    else()
      # TODO: RPM Dependencies

      set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libsdl1.2debian, libsdl-mixer1.2, libwxbase2.8-0, libwxgtk2.8-0")
      set(CPACK_DEBIAN_PACKAGE_SUGGESTS "boom-wad | doom-wad")
    endif()

    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A free, cross-platform modification of the Doom engine that allows players to easily join servers dedicated to playing Doom online.")
    set(CPACK_PACKAGE_VENDOR "Odamex Development Team")
    set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

    set(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")

    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://odamex.net")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Alex Mayfield <alexmax2742@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION Games)
  endif()
endif()

include(CPack)
