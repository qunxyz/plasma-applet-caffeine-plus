import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.components 2.0
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

Item {
	id: root
	property var icon: plasmoid.configuration.iconInactive

    Plasmoid.switchWidth: 300
    Plasmoid.switchHeight: 400

	Component.onCompleted: {
		console.log("#############autostart")
		console.log(plasmoid.configuration.autostart)
		console.log("#############enableFullscreen")
		console.log(plasmoid.configuration.enableFullscreen)
		console.log("#############useDefaultIcons")
		console.log(plasmoid.configuration.useDefaultIcons)
		console.log("#############isShowIndicator")
		console.log(plasmoid.configuration.isShowIndicator)
		console.log("#############enableNotifications")
		console.log(plasmoid.configuration.enableNotifications)
		console.log("#############enableRestore")
		console.log(plasmoid.configuration.enableRestore)
		console.log("#############iconActive")
		console.log(plasmoid.configuration.iconActive)
		console.log("#############iconInactive")
		console.log(plasmoid.configuration.iconInactive)
		console.log("#############isInhibited")
		var inst = caffeinePlus.getInstance()
		if ( inst ) {
			console.log(inst.isInhibited())
		} else {
			console.log("error happened")
		}
    }
    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.compactRepresentation: PlasmaCore.IconItem {
        source: icon
        width: units.iconSizes.medium
        height: units.iconSizes.medium
        active: mouseArea.containsMouse

        PlasmaCore.ToolTipArea {
            anchors.fill: parent
            icon: parent.source
            mainText: title
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
            	plasmoid.expanded = !plasmoid.expanded
            	console.log("############################################################")
            	if (parent.source == plasmoid.configuration.iconActive) {
            		parent.source = plasmoid.configuration.iconInactive
            	} else {
            		parent.source = plasmoid.configuration.iconActive
            	}
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
            initialPage: Qt.createComponent("Menu.qml")
        }
    }
	CaffeinePlus {
        id: caffeinePlus
    }
}
