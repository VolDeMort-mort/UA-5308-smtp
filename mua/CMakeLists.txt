find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_AUTOMOC ON)

qt_add_executable(smtp_mua
    main.cpp
    core/CommandQueue.cpp
    core/MailWorker.cpp
    bridge/MuaBridge.cpp
)

qt_add_qml_module(smtp_mua
    URI SmtpMua
    VERSION 1.0
    QML_FILES main.qml
)

target_include_directories(smtp_mua PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/bridge
    ${CMAKE_SOURCE_DIR}/database/Entity
)

target_link_libraries(smtp_mua PRIVATE
    Qt6::Quick
    smtp_proto
    Boost::headers
    Threads::Threads
)
