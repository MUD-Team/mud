##########################################
# Edge Classic - CMake Utilities
##########################################

set (EDGE_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/libraries)

# target for generating epk packs
add_custom_target(GenerateEPKS)

# defs
set (EPK_BASE_SOURCE
    "edge_defs"    
) 

function( add_epk EPK_PATH )
    add_custom_command(
        TARGET GenerateEPKS
        COMMAND ${CMAKE_COMMAND} -E tar "cf" "${CMAKE_SOURCE_DIR}/${EPK_PATH}.epk" --format=zip "."
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/${EPK_PATH}"
    )
endfunction()

foreach( epk ${EPK_BASE_SOURCE})
    add_epk(${epk})
endforeach( epk )

