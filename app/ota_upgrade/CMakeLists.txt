set(file
    ota_upgrade.c
    chk.c
    crc.c
)

add_executable(ota_upgrade ${file})
target_include_directories(ota_upgrade PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ota_upgrade PUBLIC ${PROJECT_SOURCE_DIR}/app/ar8030)
target_link_libraries(ota_upgrade PUBLIC ar8030_client)

if(NOT WIN32)
    set_target_properties(ota_upgrade PROPERTIES
        BUILD_RPATH "\$ORIGIN/../ar8030"
        INSTALL_RPATH "\$ORIGIN/../lib"
    )
endif()

INSTALL(TARGETS ota_upgrade
    RUNTIME DESTINATION bin
)
