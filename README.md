## plasma-applet-caffeine-plus

Fill the cup to inhibit auto suspend and screensaver.

![Screenshot](https://github.com/qunxyz/gnome-shell-extension-caffeine/raw/master/screenshot.png)

![Preferences](https://github.com/qunxyz/gnome-shell-extension-caffeine/raw/master/screenshot-prefs.png)

White: Empty cup = normal auto suspend and screensaver. Filled cup = auto suspend and screensaver off.
Green: No empty cup status. Filled cup = auto suspend and screensaver off always.

## Requirement
take fedora as example, build caffeine plus need below packages: 
cmake extra-cmake-modules qt5-qtbase-devel qt5-qtquickcontrols2-devel kf5-plasma-devel kf5-kio-devel

## Installation from git

    git clone git://github.com/qunxyz/plasma-applet-caffeine-plus.git
    cd plasma-applet-caffeine-plus
    ./install.sh
    ./shell-restart.sh

Then enable the extension.

## Uninstallation

To uninstall after installing from git:

    cd build
    sudo make uninstall
    cd ..
    ./shell-restart.sh

## Development

    git clone git://github.com/qunxyz/plasma-applet-caffeine-plus.git
    cd plasma-applet-caffeine-plus
    ./dev-build.sh
