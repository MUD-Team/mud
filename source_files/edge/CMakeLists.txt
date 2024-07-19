##########################################
# edge
##########################################

set (EDGE_SOURCE_FILES
  i_movie.cc
  i_ctrl.cc
  i_video.cc
  i_sound.cc
  bot_nav.cc
  bot_think.cc
  con_con.cc
  con_main.cc
  con_var.cc
  e_input.cc
  e_main.cc
  e_player.cc
  g_game.cc
  i_system.cc
  im_data.cc
  im_funcs.cc
  m_argv.cc
  m_bbox.cc
  m_cheat.cc
  m_math.cc
  m_misc.cc
  m_netgame.cc
  m_random.cc
  n_network.cc
  p_action.cc
  p_blockmap.cc
  p_enemy.cc
  p_inter.cc
  p_lights.cc
  p_map.cc
  p_maputl.cc
  p_mobj.cc
  p_plane.cc
  p_setup.cc
  p_sight.cc
  p_spec.cc
  p_switch.cc
  p_tick.cc
  p_user.cc
  p_forces.cc
  p_telept.cc
  p_weapon.cc
  r_draw.cc
  r_shader.cc
  r_render.cc
  r_effects.cc
  r_main.cc
  r_occlude.cc
  r_things.cc
  r_units.cc
  r_misc.cc
  r_sky.cc
  r_state.cc
  r_colormap.cc
  r_modes.cc
  r_image.cc
  r_doomtex.cc
  r_texgl.cc
  s_blit.cc
  s_cache.cc
  s_sound.cc
  s_music.cc
  s_ogg.cc
  s_fluid.cc
  s_wav.cc
  sv_main.cc
  snd_data.cc
  snd_gather.cc
  snd_types.cc
  w_files.cc
  w_flat.cc
  w_epk.cc
  w_sprite.cc)

set (EDGE_SOURCE_FILES ${EDGE_SOURCE_FILES} i_main.cc)

if (WIN32)
  set (EDGE_SOURCE_FILES ${EDGE_SOURCE_FILES} w32_res.rc)
endif()

set (EDGE_LINK_LIBRARIES
  edge_ajbsp  
  edge_ddf
  edge_epi  
  fluidlite
  sokol
  physfs-static
)

if (APPLE)
  set (EDGE_LINK_LIBRARIES ${EDGE_LINK_LIBRARIES} "-framework AudioToolbox")
elseif (UNIX)
  set (EDGE_LINK_LIBRARIES ${EDGE_LINK_LIBRARIES} asound)
endif()

if (EDGE_PROFILING)
  set (EDGE_LINK_LIBRARIES ${EDGE_LINK_LIBRARIES} TracyClient)
endif()

add_executable(
  edge-classic
  ${EDGE_SOURCE_FILES}
)

if(WIN32)
  target_compile_definitions(edge-classic PRIVATE WIN32)
else()
  target_compile_definitions(edge-classic PRIVATE UNIX)
endif()

target_include_directories(edge-classic PRIVATE ./)

target_include_directories(edge-classic PRIVATE ${EDGE_LIBRARY_DIR}/glad/include/glad)
set(EDGE_LINK_LIBRARIES ${EDGE_LINK_LIBRARIES} glad ${OPENGL_LIBRARIES})

target_include_directories(edge-classic SYSTEM PRIVATE ${EDGE_LIBRARY_DIR}/almostequals)
target_include_directories(edge-classic PRIVATE ../ajbsp)
target_include_directories(edge-classic PRIVATE ../ddf)
target_include_directories(edge-classic SYSTEM PRIVATE ${EDGE_LIBRARY_DIR}/dr_libs)
target_include_directories(edge-classic PRIVATE ../epi)
target_include_directories(edge-classic SYSTEM PRIVATE ${EDGE_LIBRARY_DIR}/fluidlite/include)
target_include_directories(edge-classic SYSTEM PRIVATE ${EDGE_LIBRARY_DIR}/hmm)
target_include_directories(edge-classic SYSTEM PRIVATE ${EDGE_LIBRARY_DIR}/pl_mpeg)
target_include_directories(edge-classic SYSTEM PRIVATE ${EDGE_LIBRARY_DIR}/stb)

target_link_libraries(edge-classic PRIVATE ${EDGE_LINK_LIBRARIES})

target_compile_options(edge-classic PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:${EDGE_WARNINGS}>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:${EDGE_WARNINGS}>
)

set(COPY_FILES "")

set (DEST_DIR "${CMAKE_SOURCE_DIR}")
list(APPEND COPY_FILES "$<TARGET_FILE:edge-classic>")

if (COPY_FILES)
  add_custom_command( TARGET edge-classic POST_BUILD COMMAND_EXPAND_LISTS COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COPY_FILES} ${DEST_DIR})
endif()

