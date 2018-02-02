/*
 * Copyright 2015  Martin Kotelnik <clearmartin@seznam.cz>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http: //www.gnu.org/licenses/>.
 */
import QtQuick 2.2
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.0
import QtQuick.Window 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

Item {
    id: advancedConfig
    property var cfg_userApps:[]
    signal configurationChanged

	Component.onCompleted: {
		var user_apps = cfg_userApps
		for (var i = 0; i < user_apps.length; i++) {
        	var user_app = user_apps[i]
        	var isContinue = false
        	for (var j = 0; j < container.children.length; j++) {
        		if (user_app == container.children[j].children[0].text) {
					isContinue = true
					break
				}
        	}
        	if (isContinue) continue
            var component = Qt.createComponent("app.qml")
			var buttonRow = component.createObject(container)
			buttonRow.children[0].text = user_app
		}
    }

    FileDialog {
        id: fileDialog
        visible: false
        modality: Qt.WindowModal
        title: "Choose some files"
        selectExisting: true
        selectMultiple: false
        selectFolder: false
        nameFilters: [ "Desktop files (*.desktop)", "All files (*)" ]
        selectedNameFilter: "All files (*)"
        sidebarVisible: true
        onAccepted: {
            console.log("Accepted: " + fileUrls)
            console.log("container: " + container.children.length)
            if ( container.children.length )
            	console.log("container button text: " + container.children[0].children[0].text)
            var userApps = new Array()
            for (var i = 0; i < fileUrls.length; i++) {
            	var fileUrl = fileUrls[i]
            	var isContinue = false
            	for (var j = 0; j < container.children.length; j++) {
            		if (fileUrl == container.children[j].children[0].text) {
						isContinue = true
						break
					}
            	}
            	if (isContinue) continue
	            var component = Qt.createComponent("app.qml")
				var buttonRow = component.createObject(container)
				buttonRow.children[0].text = fileUrl
				cfg_userApps.push(fileUrl)
			}
			advancedConfig.configurationChanged()
        }
        onRejected: { console.log("Rejected") }
    }

    PlasmaExtras.ScrollArea {
        anchors.fill: parent
        width: parent.width
        Flickable {
            id: flickable
            contentWidth: container.width
            contentHeight: container.height
            clip: true
            anchors.fill: parent

            Item {
                width: Math.max(flickable.width, container.width)
                height: container.height
                Column {
                    id: container
                    spacing: 20
                }
            }
        }
    }

    Rectangle {
        id: bottomBar
        anchors {
            bottom: parent.bottom
        }
        height: buttonRow.height * 1.2
        Row {
            id: buttonRow
            Button {
                text: "Add app need inhibit screensaver"
                onClicked: fileDialog.open()
            }
        }
    }
}
