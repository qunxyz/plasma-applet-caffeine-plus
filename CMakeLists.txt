# Set minimum CMake version (required for CMake 3.0 or later)
# "Your project should require at least CMake 3.16.0 to use FindKF5.cmake"
cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

project(plasma-applet-caffeine-plus)

set(QT_MIN_VERSION "5.4.0")
set(KF5_MIN_VERSION "5.0.0")

# Use Extra CMake Modules (ECM) for common functionality.
# See http://api.kde.org/ecm/manual/ecm.7.html
# and http://api.kde.org/ecm/manual/ecm-kde-modules.7.html
find_package(ECM REQUIRED NO_MODULE)

# Needed by find_package(KF5Plasma) below.
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

# Locate plasma_install_package macro.
find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Quick)
find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED DBus)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Plasma)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS WindowSystem)

add_subdirectory(plugin)
add_subdirectory(translations)

# Add installatation target ("make install").
plasma_install_package(plasmoid org.kde.plasma.caffeine-plus)

install( DIRECTORY image/hicolor DESTINATION ${ICON_INSTALL_DIR})

find_program(XDG-ICON-RESOURCE_EXECUTABLE xdg-icon-resource)

EXECUTE_PROCESS(COMMAND ${XDG-ICON-RESOURCE_EXECUTABLE} forceupdate)
