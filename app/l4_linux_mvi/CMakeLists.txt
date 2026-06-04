set(file
    l4_linux_mvi.c
)

add_executable(l4_linux_mvi ${file})
target_include_directories(l4_linux_mvi PUBLIC ar8030)
target_link_libraries(l4_linux_mvi PUBLIC ar8030_client)

if(NOT WIN32)
    set_target_properties(l4_linux_mvi PROPERTIES
        BUILD_RPATH "\$ORIGIN/../ar8030"
        INSTALL_RPATH "\$ORIGIN/../lib"
    )
endif()

INSTALL(TARGETS l4_linux_mvi
    RUNTIME DESTINATION bin
)
