if(NOT WIN32)
    set_source_files_properties(cmd_dbg.cpp PROPERTIES COMPILE_FLAGS "-std=c++14")
endif()

add_executable(l4_cmd_dbg cmd_dbg.cpp)
target_link_libraries(l4_cmd_dbg PUBLIC ar8030_client)

target_link_directories(l4_cmd_dbg PUBLIC ${PROJECT_SOURCE_DIR}/com)
target_link_libraries(l4_cmd_dbg PRIVATE com)

if(NOT WIN32)
    set_target_properties(l4_cmd_dbg PROPERTIES
        BUILD_RPATH "\$ORIGIN/../ar8030"
        INSTALL_RPATH "\$ORIGIN/../lib"
    )
endif()

INSTALL(TARGETS l4_cmd_dbg
    RUNTIME DESTINATION bin
)
