set(SDL3_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include")

# Support both 32 and 64 bit builds
if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
  set(SDL3_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL3.lib")
else ()
  set(SDL3_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/lib/x86/SDL3.lib")
endif ()

string(STRIP "${SDL3_LIBRARIES}" SDL3_LIBRARIES)