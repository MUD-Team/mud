##########################################
# epi
##########################################

add_library(
  edge_epi
  epi_color.cc
  epi_ename.cc
  epi_filesystem.cc
  epi_lexer.cc
  epi_str_compare.cc
  epi_str_util.cc
)

target_link_libraries(edge_epi PRIVATE almostequals cglm physfs-static xxhash)

target_include_directories(edge_epi PUBLIC ./)

target_compile_options(edge_epi PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:${EDGE_WARNINGS}>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:${EDGE_WARNINGS}>
)
