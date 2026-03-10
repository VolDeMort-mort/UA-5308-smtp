cmake_minimum_required(VERSION 4.0)

# project(EmailClientApp VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)


qt_add_executable(smtp_mua main.cpp)


set_source_files_properties(
    qml/Singleton/Icons.qml
    qml/Singleton/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)


qt_add_qml_module(smtp_mua
    URI SmtpMua
    VERSION 1.0
    # RESOURCE_PREFIX "/"

    QML_FILES
        qml/Singleton/Theme.qml
        qml/Singleton/Icons.qml
        qml/Singleton/ThemeData.js

        qml/Components/ActionBtn.qml
        qml/Components/SvgIcon.qml
        qml/Components/SidebarItem.qml

        qml/Components/EmailDelegate.qml
        qml/Components/StyledCheckBox.qml

        qml/Views/Sidebar.qml
        qml/Views/TopSearchBar.qml
        qml/Views/Toolbar.qml
        qml/Views/EmailListView.qml
        qml/Views/ComposePopup.qml
        qml/Views/ReadMessagePopup.qml
        qml/Views/FilterPopup.qml

        qml/main.qml
        QML_FILES qml/Components/FilterModel.qml
        QML_FILES qml/Components/NavType.qml


)

target_link_libraries(smtp_mua
    PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
)
