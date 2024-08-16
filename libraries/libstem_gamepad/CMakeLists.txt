##########################################
# libstem_gamepad
##########################################

set (LSGP_SOURCE_FILES
  source/gamepad/Gamepad_db.c
  source/gamepad/Gamepad_private.c
)

if (APPLE)
  set (LSGP_SOURCE_FILES ${LSGP_SOURCE_FILES} source/gamepad/Gamepad_macosx.c)
elseif (UNIX)
  set (LSGP_SOURCE_FILES ${LSGP_SOURCE_FILES} source/gamepad/Gamepad_linux.c)
else ()
  set (LSGP_SOURCE_FILES ${LSGP_SOURCE_FILES} source/gamepad/Gamepad_windows_dinput.c)
endif()

add_library(
  libstem_gamepad
  ${LSGP_SOURCE_FILES}
)

target_include_directories(libstem_gamepad PUBLIC source)