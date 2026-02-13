find_package(Qt6 REQUIRED COMPONENTS Quick)

set(CMAKE_AUTOMOC ON)

qt_add_executable(smtp_mua main.cpp)
qt_add_qml_module(smtp_mua URI SmtpMua VERSION 1.0 QML_FILES main.qml)
target_link_libraries(smtp_mua PRIVATE Qt6::Quick smtp_proto)
