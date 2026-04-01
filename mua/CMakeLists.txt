find_package(Qt6 QUIET OPTIONAL_COMPONENTS Quick Qml)

set(MUA_CORE_SOURCES
    core/CommandQueue.cpp
    core/MailWorker.cpp
)

add_library(mua_core STATIC
    ${MUA_CORE_SOURCES}
)

target_include_directories(mua_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

target_link_libraries(mua_core PUBLIC
    smtp_proto
    mime_lib
    base64_lib
)

add_executable(smtp_mua_console
    mua_console_main.cpp
)

add_executable(mua_seed_users
    tools/seed_users.cpp
)

target_link_libraries(smtp_mua_console PRIVATE mua_core)
target_link_libraries(mua_seed_users PRIVATE db_lib config_lib)

target_include_directories(mua_seed_users PRIVATE
    ${PROJECT_SOURCE_DIR}/database
)

if (TARGET Qt6::Quick)
    qt_add_executable(smtp_mua
        main.cpp
        bridge/MuaBridge.cpp
    )
    set_property(TARGET smtp_mua PROPERTY AUTOMOC ON)
    set_property(TARGET smtp_mua PROPERTY AUTORCC ON)

    qt_add_qml_module(smtp_mua URI SmtpMua VERSION 1.0 QML_FILES main.qml)
    target_link_libraries(smtp_mua PRIVATE Qt6::Quick mua_core)

    target_include_directories(smtp_mua PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/bridge
    )
endif()
