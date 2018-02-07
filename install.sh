#!/bin/bash

SYS_TYPE=""
PKG_CMD=""

if [ -f /usr/bin/apt ]; then
	SYS_TYPE="Debian"
	PKG_CMD="apt-get"
	#${PKG_CMD} install cmake extra-modules build-essential gettext qtdeclarative5-dev libkf5kio-dev plasma-framework-dev
	sudo ${PKG_CMD} install cmake extra-cmake-modules build-essential gettext qtdeclarative5-dev libkf5kio-dev plasma-framework-dev
elif [[ -f /usr/bin/dnf || -L /usr/bin/dnf ]]; then
	SYS_TYPE="Fedora"
	PKG_CMD="dnf"
	sudo ${PKG_CMD} install cmake extra-cmake-modules kf5-plasma-devel kf5-kio-devel gettext
elif [ -f /usr/bin/yum ]; then
	SYS_TYPE="RedHat"
	PKG_CMD="yum"
elif [ -f /usr/bin/pacman ]; then
	SYS_TYPE="ArchLinux"
	PKG_CMD="pacman"
fi

#echo "SYS_TYPE: ${SYS_TYPE} PKG_CMD: ${PKG_CMD}"

#exit
##########################################

DIR="$( dirname "${BASH_SOURCE[0]}" )"
cd $DIR

if [[ -d "${DIR}/build" ]]; then
    cd "${DIR}/build" && sudo make uninstall
    cd ..
    sudo rm -r build
fi
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release -DKDE_INSTALL_USE_QT_SYS_PATHS=ON && make && sudo make install

if [[ -z $XDG_CACHE_HOME && -f "${XDG_CACHE_HOME}/icon-cache.kcache" ]]; then
	rm ${XDG_CACHE_HOME}/icon-cache.kcache
elif [[ -f "${HOME}/.cache/icon-cache.kcache" ]]; then
	rm ${HOME}/.cache/icon-cache.kcache
fi
gtk-update-icon-cache
kbuildsycoca5 --noincremental