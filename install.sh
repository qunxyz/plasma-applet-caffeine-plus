#!/bin/bash

SYS_TYPE=""
PKG_CMD=""

if [ -f /usr/bin/apt ]; then
	SYS_TYPE="Debian"
	PKG_CMD="apt-get"
	sudo ${PKG_CMD} install g++ cmake extra-cmake-modules pkg-config gettext qtdeclarative5-dev libkf5kio-dev libkf5plasma-dev
elif [[ -f /usr/bin/dnf || -L /usr/bin/dnf ]]; then
	SYS_TYPE="Fedora"
	PKG_CMD="dnf"
	sudo ${PKG_CMD} install cmake extra-cmake-modules qt5-qtdeclarative-devel kf5-plasma-devel kf5-kio-devel gettext
elif [ -f /usr/bin/zypper ]; then
	SYS_TYPE="openSUSE"
	PKG_CMD="zypper"
	INSTALL_PREFIX="-DCMAKE_INSTALL_PREFIX=/usr"
	sudo ${PKG_CMD} install gcc cmake extra-cmake-modules libqt5-qtdeclarative-devel plasma-framework-devel kio-devel kwindowsystem-devel AppStream
elif [ -f /usr/bin/pacman ]; then
	SYS_TYPE="ArchLinux"
	PKG_CMD="pacman"
	sudo ${PKG_CMD} -S gcc make cmake extra-cmake-modules pkg-config
fi

DIR="$( dirname "${BASH_SOURCE[0]}" )"
cd $DIR

if [[ -d "${DIR}/build" ]]; then
    cd "${DIR}/build" && sudo make uninstall
    cd ..
    sudo rm -r build
fi
mkdir build
cd build

cmake .. ${INSTALL_PREFIX} -DCMAKE_BUILD_TYPE=Release -DKDE_INSTALL_USE_QT_SYS_PATHS=ON && make && sudo make install

if [[ -z $XDG_CACHE_HOME && -f "${XDG_CACHE_HOME}/icon-cache.kcache" ]]; then
	rm ${XDG_CACHE_HOME}/icon-cache.kcache
elif [[ -f "${HOME}/.cache/icon-cache.kcache" ]]; then
	rm ${HOME}/.cache/icon-cache.kcache
fi
gtk-update-icon-cache
kbuildsycoca5 --noincremental
