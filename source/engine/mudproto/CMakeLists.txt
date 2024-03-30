find_package(protobuf CONFIG REQUIRED)

set(MUDPROTO_PROTOS common.proto server.proto)

protobuf_generate(OUT_VAR MUDPROTO_PROTOS PROTOS ${MUDPROTO_PROTOS})

add_library(mudproto STATIC ${MUDPROTO_PROTOS} ${MUDPROTO_SOURCES})
target_include_directories(mudproto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(mudproto protobuf::libprotobuf)

if(MSVC)
  # https://developercommunity.visualstudio.com/t/Visual-Studio-1740-no-longer-compiles-/10193665
  target_compile_definitions(mudproto PUBLIC "_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS")
endif()
