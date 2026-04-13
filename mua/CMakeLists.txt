cmake_minimum_required(VERSION 3.24)

find_package(Qt6 QUIET OPTIONAL_COMPONENTS Quick Qml QuickControls2)

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
			qml/Components/MailListDelegate.qml
			qml/Components/StyledCheckBox.qml
			qml/Components/FilterModel.qml
			qml/Components/NavType.qml
			qml/Components/MailListView.qml
			qml/Components/MailView.qml
			qml/Components/StyledInput.qml


			qml/Views/Sidebar.qml
			qml/Views/TopSearchBar.qml
			qml/Views/Toolbar.qml
			qml/Views/MailArea.qml
			qml/Views/ComposePopup.qml
			qml/Views/FilterPopup.qml
			qml/Views/LoginView.qml
			qml/Views/SignInView.qml

			qml/main.qml
	)
	
    target_link_libraries(smtp_mua PRIVATE Qt6::Quick Qt6::QuickControls2 mua_core)

    target_include_directories(smtp_mua PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/bridge
    )
endif()
