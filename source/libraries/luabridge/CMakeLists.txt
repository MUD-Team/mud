

add_library(luabridge INTERFACE)
target_link_libraries(luabridge INTERFACE lua)
target_include_directories(luabridge INTERFACE . )
