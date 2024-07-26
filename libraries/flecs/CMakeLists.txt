
file(GLOB_RECURSE FLECS_INCLUDE include/*.h include/*.hpp)
file(GLOB_RECURSE FLECS_SOURCE src/*.c)

add_library(flecs STATIC ${FLECS_SOURCE} ${FLECS_INCLUDE})

target_compile_definitions(flecs PUBLIC flecs_STATIC)
target_include_directories(flecs PUBLIC ./include)