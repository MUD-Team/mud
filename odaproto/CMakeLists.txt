find_package(protobuf CONFIG REQUIRED)

set(ODAPROTO_PROTOS common.proto server.proto)
set(ODAPROTO_SOURCES common.pb.cc server.pb.cc)

source_group("Definitions" FILES ${ODAPROTO_PROTOS})
source_group("Generated" FILES ${ODAPROTO_SOURCES})

add_library(odaproto STATIC ${ODAPROTO_SOURCES} ${ODAPROTO_PROTOS})
target_include_directories(odaproto PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(odaproto protobuf::libprotobuf)

if(MSVC)
  # https://developercommunity.visualstudio.com/t/Visual-Studio-1740-no-longer-compiles-/10193665
  target_compile_definitions(odaproto PUBLIC "_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS")
endif()
