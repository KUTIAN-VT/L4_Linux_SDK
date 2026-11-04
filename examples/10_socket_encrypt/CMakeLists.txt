add_executable(l4_socket_encrypt
    socket_encrypt.c
)

target_link_libraries(l4_socket_encrypt
    PRIVATE
        l4_example_common
)

if(NOT WIN32)
    set_target_properties(l4_socket_encrypt PROPERTIES
        BUILD_RPATH "\$ORIGIN/../ar8030"
        INSTALL_RPATH "\$ORIGIN/../lib"
    )
endif()

INSTALL(TARGETS l4_socket_encrypt
    RUNTIME DESTINATION bin
)
