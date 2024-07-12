
set(system_libs "")

if (APPLE)
    set(system_libs ${system_libs} "-framework QuartzCore" "-framework Cocoa" "-framework AudioToolbox" "-framework OpenGL")    
elseif(LINUX)
    set(system_libs X11 Xi Xcursor GL asound dl m)
endif()

add_library(sokol sokol.cc)
target_include_directories(sokol PUBLIC ./)
target_link_libraries(sokol ${system_libs})

if (APPLE)
    target_compile_options(sokol PRIVATE -x objective-c++)
endif()