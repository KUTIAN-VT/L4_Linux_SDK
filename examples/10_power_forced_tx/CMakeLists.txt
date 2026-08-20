add_executable(l4_power_forced_tx
    power_forced_tx.c
)

target_link_libraries(l4_power_forced_tx
    PRIVATE
        l4_example_common
)

if(NOT WIN32)
    set_target_properties(l4_power_forced_tx PROPERTIES
        BUILD_RPATH "\$ORIGIN/../ar8030"
        INSTALL_RPATH "\$ORIGIN/../lib"
    )
endif()

INSTALL(TARGETS l4_power_forced_tx
    RUNTIME DESTINATION bin
)
