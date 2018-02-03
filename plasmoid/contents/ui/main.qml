import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.components 2.0
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.private.CaffeinePlus 1.0 as CaffeinePlus

//import "../code/layout.js" as LayoutManager

Item {
	id: root
	property var icon: plasmoid.configuration.iconInactive

    Plasmoid.switchWidth: 300
    Plasmoid.switchHeight: 400
/////////////////////////////////////////////////////////////
    //Layout.minimumWidth: LayoutManager.minimumWidth()
    //Layout.minimumHeight: LayoutManager.minimumHeight()
    //Layout.preferredWidth: LayoutManager.preferredWidth()
    //Layout.preferredHeight: LayoutManager.preferredHeight()
/*
    function action_addLauncher()
    {
        caffeinePlus.addLauncher();
    }*/
/*
    PlasmaCore.Dialog {
        id: popup
        type: PlasmaCore.Dialog.PopupMenu
        flags: Qt.WindowStaysOnTopHint
        hideOnWindowDeactivate: true
        location: plasmoid.location
        visualParent: vertical ? popupArrow : root

        mainItem: Popup { }
    }*/
///////////////////////////////////////////
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
		console.log("#############userApps")
		console.log(plasmoid.configuration.userApps)
		caffeinePlus.init(plasmoid.configuration.enableFullscreen, plasmoid.configuration.userApps)
        //plasmoid.setAction("addLauncher", i18n("Add Launcher..."), "list-add");//////////////////////////////


		caffeinePlus.toggle(plasmoid.configuration.enableRestore)
    }
    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.compactRepresentation: PlasmaCore.IconItem {
    	id: sysTray
        source: icon
        width: units.iconSizes.medium
        height: units.iconSizes.medium
        active: mouseArea.containsMouse

        function toggleIcon() {
			//var inst = caffeinePlus.getInstance()
			//if ( inst.isInhibited() ) {
			if ( caffeinePlus.isInhibited() ) {
            	//console.log("#####################isInhibited true#############################")
            	if ( plasmoid.configuration.enableRestore ) {
					sysTray.source = plasmoid.configuration.iconUserActive
            	} else {
					sysTray.source = plasmoid.configuration.iconActive
            	}
			} else {
            	//console.log("#####################isInhibited false#############################")
				sysTray.source = plasmoid.configuration.iconInactive
			}
        }

        PlasmaCore.ToolTipArea {
            anchors.fill: parent
            icon: parent.source
            mainText: "testing title"
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
            	plasmoid.expanded = !plasmoid.expanded
            }
            hoverEnabled: true
        }

        Timer {
        	id: textTimer
        	interval: 1000
        	repeat: true
        	running: true
        	triggeredOnStart: true
        	onTriggered: sysTray.toggleIcon()
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
            //initialPage: Qt.createComponent("list.qml")
            initialPage: Qt.createComponent("windows.qml")
        }
    }
    CaffeinePlus.CaffeinePlus{
        id: caffeinePlus

	    function toggle(flag) {
	        if (flag) {
	            caffeinePlus.addInhibition("user", "inhibit by caffeine plus")
	        } else {
	            caffeinePlus.releaseInhibition("user")
	        }
	    }
    }
}
