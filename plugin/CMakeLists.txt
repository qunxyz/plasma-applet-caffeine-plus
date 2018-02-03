find_package(KF5KIO REQUIRED)

set(plasmoidplugin_SRCS
    plasmoidplugin.cpp
    caffeine-plus.cpp
)
include_directories(
    ${CMAKE_BINARY_DIR}
)
message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

add_library(plasmoidplugin SHARED ${plasmoidplugin_SRCS})

target_link_libraries(plasmoidplugin Qt5::Quick KF5::Plasma Qt5::DBus KF5::WindowSystem Qt5::Core Qt5::Qml KF5::KIOCore KF5::KIOWidgets)

install(TARGETS plasmoidplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/private/CaffeinePlus)

install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/private/CaffeinePlus)