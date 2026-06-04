

set(file
    normal_node.c
    sock_node_rpc.c
    rpc_dev_bind.c
    usb_event_list.c
    rpc_init.c
    rpc_dev_proc.c
    callback_node.c
    sock_node.c
    base_node.c
    dev_event_node.c
    rpc_debug.c
    rpc_cmdline.c
    rpc_platform_ctrl.c
)

if(DAEMON_STATIC_LIB)
    add_library(daemon STATIC ${file})
else()
    add_executable(daemon main.c ${file})
endif()

if(WIN32)
    install(FILES $<TARGET_PDB_FILE:daemon> DESTINATION bin OPTIONAL)

    target_link_libraries(daemon PRIVATE "Ws2_32.lib")
    set_source_files_properties(main.c PROPERTIES LANGUAGE CXX)
    target_link_libraries(daemon PRIVATE "pthread.lib")
elseif(NOT ANDROID)
    target_link_libraries(daemon PRIVATE -lpthread)
endif()

target_link_directories(daemon PUBLIC ${PROJECT_SOURCE_DIR}/com)
target_link_libraries(daemon PRIVATE com)

add_subdirectory(dev8030)
target_include_directories(daemon PRIVATE "dev8030")
target_include_directories(daemon PRIVATE ./)
target_link_directories(daemon PUBLIC dev8030)
target_link_libraries(daemon PRIVATE dev8030)

INSTALL(TARGETS daemon
    RUNTIME DESTINATION bin
)

if(ANDROID)
    add_subdirectory(android)
endif()
