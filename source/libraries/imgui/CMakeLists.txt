set (IMGUI_SOURCES
    imgui_demo.cpp 
    imgui_draw.cpp 
    imgui_tables.cpp 
    imgui_widgets.cpp 
    imgui.cpp
    backend/imgui_impl_sdl2.cpp
    backend/imgui_impl_sdlrenderer2.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_link_libraries(imgui SDL2::SDL2)
target_include_directories(imgui PUBLIC ./)