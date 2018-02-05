#!/bin/bash

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

../shell-restart.sh &