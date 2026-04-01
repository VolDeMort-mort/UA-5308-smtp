find_package(Qt6 QUIET COMPONENTS Quick Qml)

set(MUA_CORE_SOURCES
    core/CommandQueue.cpp
    core/MailWorker.cpp
)

add_executable(smtp_mua_console
    mua_console_main.cpp
    ${MUA_CORE_SOURCES}
)

add_executable(mua_seed_users
    tools/seed_users.cpp
)

target_link_libraries(smtp_mua_console PRIVATE smtp_proto mime_lib base64_lib)
target_link_libraries(mua_seed_users PRIVATE db_lib config_lib)

target_include_directories(smtp_mua_console PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${PROJECT_SOURCE_DIR}/proto
    ${PROJECT_SOURCE_DIR}/base64/include
    ${PROJECT_SOURCE_DIR}/mime/include
)

target_include_directories(mua_seed_users PRIVATE
    ${PROJECT_SOURCE_DIR}/database
)

if (Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    qt_add_executable(smtp_mua
        main.cpp
        bridge/MuaBridge.cpp
        ${MUA_CORE_SOURCES}
    )
    qt_add_qml_module(smtp_mua URI SmtpMua VERSION 1.0 QML_FILES main.qml)
    target_link_libraries(smtp_mua PRIVATE Qt6::Quick smtp_proto mime_lib base64_lib)

    target_include_directories(smtp_mua PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/bridge
        ${PROJECT_SOURCE_DIR}/proto
        ${PROJECT_SOURCE_DIR}/base64/include
        ${PROJECT_SOURCE_DIR}/mime/include
    )
endif()
