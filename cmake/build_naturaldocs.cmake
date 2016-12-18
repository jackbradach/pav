# Builds documentation for project
function(build_naturaldocs
                proj_name
                nd_input
                nd_output
                nd_project_dir
)

# Generate HTML documentation using Natural Docs
find_program(ND_BIN NAMES naturaldocs)
if(NOT EXISTS ${ND_BIN})
        message(WARNING "NaturalDocs not found, skipping generation of HTML documentation")
        return()
endif()

set(ND_INPUT_DIR ${nd_input})
set(ND_OUTPUT_DIR ${nd_output})
set(ND_PROJECT_DIR ${nd_project_dir})

file(MAKE_DIRECTORY ${ND_PROJECT_DIR})
file(MAKE_DIRECTORY ${ND_OUTPUT_DIR})

add_custom_target(doc
        ALL
        COMMAND ${ND_BIN}
                -q
                -i ${nd_input}
                -o HTML ${nd_output}
                -p ${nd_project_dir}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating documentation using NaturalDocs..."
)
install(DIRECTORY ${nd_output}/
        DESTINATION "share/doc/${proj_name}"
        PATTERN "${nd_output}/*"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

endfunction(build_naturaldocs)
