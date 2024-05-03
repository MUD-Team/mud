
file(GLOB COMMON_HEADERS *.h)
file(GLOB COMMON_SOURCES *.cpp)
file(GLOB COMMON_SCRIPT_SOURCES script/private/*.cpp)

configure_file(git_describe.h.in "${CMAKE_CURRENT_BINARY_DIR}/git_describe.h")

add_library(engine-common INTERFACE)
target_sources(engine-common INTERFACE ${COMMON_SOURCES} ${COMMON_SCRIPT_SOURCES} ${COMMON_HEADERS})
target_link_libraries(engine-common INTERFACE lua luabridge)
target_include_directories(engine-common INTERFACE . ${CMAKE_CURRENT_BINARY_DIR} ../../libraries/stb)

if(UNIX)
  include(CheckSymbolExists)
  check_symbol_exists(backtrace "execinfo.h" HAVE_BACKTRACE)

  if(HAVE_BACKTRACE)
    target_compile_definitions(engine-common INTERFACE HAVE_BACKTRACE)
  endif()
endif()
