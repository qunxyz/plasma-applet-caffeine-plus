import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.components 2.0
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.private.CaffeinePlus 1.0 as CaffeinePlus

Item {
	id: root
	property var icon: plasmoid.configuration.useDefaultIcons ? plasmoid.configuration.defaultIconInactive : plasmoid.configuration.iconInactive
	property var userApps: []
	property var enableFullscreen
	property var enableDebug
	property var windows
	property var windowsIsInited: false

    Plasmoid.switchWidth: 300
    Plasmoid.switchHeight: 400

    Plasmoid.onUserConfiguringChanged: {
    	if ( !caffeinePlus.isListEqual(root.userApps, plasmoid.configuration.userApps)
    		|| root.enableFullscreen != plasmoid.configuration.enableFullscreen
    		|| root.enableDebug != plasmoid.configuration.enableDebug ) {
    		root.userApps = plasmoid.configuration.userApps
    		root.enableFullscreen = plasmoid.configuration.enableFullscreen
    		root.enableDebug = plasmoid.configuration.enableDebug
    		caffeinePlus.updateSettings(plasmoid.configuration.enableFullscreen, plasmoid.configuration.userApps, plasmoid.configuration.enableDebug)
    	}
    }

	Component.onCompleted: {
		root.userApps = plasmoid.configuration.userApps
		root.enableFullscreen = plasmoid.configuration.enableFullscreen

		caffeinePlus.init(plasmoid.configuration.enableFullscreen, plasmoid.configuration.userApps, plasmoid.configuration.enableDebug)
		caffeinePlus.toggle(plasmoid.configuration.enableRestore)
    }
    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.compactRepresentation: PlasmaCore.IconItem {
    	id: sysTray
        source: icon
        width: units.iconSizes.medium
        height: units.iconSizes.medium
        active: mouseArea.containsMouse

		Component.onCompleted: {
			caffeinePlus.sysTray = sysTray
		}

        PlasmaCore.ToolTipArea {
            anchors.fill: parent
            icon: parent.source
            mainText: i18n("Caffeine Plus")
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
            	plasmoid.expanded = !plasmoid.expanded
            }
            hoverEnabled: true
        }
    }


    Plasmoid.fullRepresentation: Item {
        Layout.minimumWidth: 300
        Layout.minimumHeight: 400

        ToolBar {
            id: toolBar
            z: 10
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
        }

        PageStack {
            id: pageStack
            toolBar: toolBar
            clip: true
            anchors {
                top: toolBar.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            initialPage: Qt.createComponent("windows.qml")
        }
    }

    CaffeinePlus.CaffeinePlus{
        id: caffeinePlus
		property var sysTray
		property var preHasInhibition: false
		property var preInhibitionSize: 0

        onInhibitionsChanged: {
			if ( hasInhibition ) {
            	if ( plasmoid.configuration.enableRestore ) {
					sysTray.source = plasmoid.configuration.useDefaultIcons ? plasmoid.configuration.defaultIconUserActive : plasmoid.configuration.iconUserActive
            	} else {
					sysTray.source = plasmoid.configuration.useDefaultIcons ? plasmoid.configuration.defaultIconActive : plasmoid.configuration.iconActive
            	}
			} else {
				sysTray.source = plasmoid.configuration.useDefaultIcons ? plasmoid.configuration.defaultIconInactive : plasmoid.configuration.iconInactive
			}

			if ( hasInhibition != preHasInhibition || inhibitionSize != preInhibitionSize ) {
				preHasInhibition = hasInhibition
				preInhibitionSize = inhibitionSize

				if (root.windowsIsInited)
					root.windows.inhibitionsChanged()
			}
        }

        function isListEqual(list1, list2) {
        	var str1 = ""
        	var str2 = ""
        	for ( var i = 0; i < list1.length; i++ ) {
        		str1 += list1[i]
        	}
        	for ( var i = 0; i < list2.length; i++ ) {
        		str2 += list2[i]
        	}

	    	if (str1 != str2) return false

	    	return true
        }

	    function toggle(flag) {
	        if (flag) {
	            caffeinePlus.addInhibition("user", i18n("inhibit by caffeine plus"))
	        } else {
	            caffeinePlus.releaseInhibition("user")
	        }
	    }
    }
}
