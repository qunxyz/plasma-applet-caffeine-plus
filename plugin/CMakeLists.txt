find_package(PkgConfig REQUIRED)

function(pkg_check_variable _pkg _name)
    string(TOUPPER ${_pkg} _pkg_upper)
    string(TOUPPER ${_name} _name_upper)
    string(REPLACE "-" "_" _pkg_upper ${_pkg_upper})
    string(REPLACE "-" "_" _name_upper ${_name_upper})
    set(_output_name "${_pkg_upper}_${_name_upper}")

    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=${_name} ${_pkg}
                    OUTPUT_VARIABLE _pkg_result
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    set("${_output_name}" "${_pkg_result}" CACHE STRING "pkg-config variable ${_name} of ${_pkg}")
endfunction()

pkg_check_modules(DBUS dbus-1)
pkg_check_variable(dbus-1 interfaces_dir)
message("Path: ${DBUS_1_INTERFACES_DIR}")
find_package(KF5KIO)

set(plasmoidplugin_SRCS
    plasmoidplugin.cpp
    caffeine-plus.cpp
    )
include_directories(
    ${CMAKE_BINARY_DIR}
)
message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

#qt5_add_dbus_adaptor(plasmoidplugin_SRCS kf5_org.kde.Solid.PowerManagement.PolicyAgent.xml caffeine-plus.h CaffeinePlus)
#qt5_add_dbus_interfaces(plasmoidplugin_SRCS kf5_org.kde.Solid.PowerManagement.PolicyAgent.xml)

add_library(plasmoidplugin SHARED ${plasmoidplugin_SRCS})

target_link_libraries(plasmoidplugin Qt5::Quick KF5::Plasma Qt5::DBus KF5::WindowSystem Qt5::Core Qt5::Qml KF5::KIOCore KF5::KIOWidgets)

install(TARGETS plasmoidplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/private/CaffeinePlus)

install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/private/CaffeinePlus)