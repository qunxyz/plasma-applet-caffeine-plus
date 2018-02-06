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
import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.0
import QtQuick.Window 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.private.CaffeinePlus 1.0 as CaffeinePlus

Item {
    id: advancedConfig
    property var cfg_userApps:[]
    signal configurationChanged

    function addApp(url) {
    	if (advancedConfig.cfg_userApps.indexOf(url) != -1) return;

		appsModel.addApp(url)
		advancedConfig.cfg_userApps.push(url)
		advancedConfig.configurationChanged()
    }

    function removeApp(index) {
    	var item = appsModel.get(index);
		var app_index = advancedConfig.cfg_userApps.indexOf(item['url'])

    	advancedConfig.cfg_userApps.splice(app_index, 1)
    	appsModel.remove(index)
		advancedConfig.configurationChanged()
    }

	Component.onCompleted: {
		var user_apps = cfg_userApps
		for (var i = 0; i < user_apps.length; i++) {
			appsModel.addApp(user_apps[i]);
		}
    }

    Rectangle {
	    id: container
	    width: 300; height: 400

	    ListModel {
	        id: appsModel
	        function addApp(url) {
	        	var isExists = false
	        	for (var i = 0; i < appsModel.count; i++) {
	        		var item = appsModel.get(i);
	        		if (item['url'] == url) {
						isExists = true
						break
	        		}
				}
	        	if (isExists) return
		    	var info = caffeinePlus.launcherData(url)

				appsModel.append({
		            "name": info["applicationName"],
		            "iconName": info["iconName"],
		            "url": url
		        })
		    }
	    }

	    // The delegate for each fruit in the model:
	    Component {
	        id: listDelegate
	//! [0]
	        Item {
	//! [0]
	            id: delegateItem
	            width: listView.width; height: 20
	            clip: true

	            Item {

	                Row {
	                    PlasmaCore.IconItem {
	                    	source: iconName
	                    	width:24
	                    	height: 20
	                    }

	                    Text {
	                        text: name
	                        font.pixelSize: 14
	                        width: 200
	                        height: 20
	                    }

	                    Text {
	                    	text: url
	                    	visible: false
	                    }

	                    PlasmaComponents.Button {
	                        text: "Remove"
	                        height: 20
	                    	width:64
	                        MouseArea { anchors.fill:parent; onClicked: advancedConfig.removeApp(index) }
	                    }
	                }
	            }

	            // Animate adding and removing of items:
	//! [1]
	            ListView.onAdd: SequentialAnimation {
	                PropertyAction { target: delegateItem; property: "height"; value: 0 }
	                NumberAnimation { target: delegateItem; property: "height"; to: 40; duration: 250; easing.type: Easing.InOutQuad }
	            }

	            ListView.onRemove: SequentialAnimation {
	                PropertyAction { target: delegateItem; property: "ListView.delayRemove"; value: true }

	                // Make sure delayRemove is set back to false so that the item can be destroyed
	                PropertyAction { target: delegateItem; property: "ListView.delayRemove"; value: false }
	            }
	        }
	//! [1]
	    }

	    // The view:
	    ListView {
	        id: listView
	        flickableDirection: Flickable.VerticalFlick
	        boundsBehavior: Flickable.StopAtBounds
	        clip: true
	        anchors {
	            left: parent.left; top: parent.top;
	            right: parent.right; bottom: parent.bottom;//bottom: buttons.top;
	            margins: 0
	        }
	        model: appsModel
	        delegate: listDelegate
	         Layout.fillWidth: true
	            Layout.fillHeight: true

	            //PlasmaComponents.ScrollBar.vertical: PlasmaComponents.ScrollBar {}
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
            PlasmaComponents.Button {
                text: i18n("Add app need inhibit screensaver")
                onClicked: caffeinePlus.addLauncher()
            }
        }
    }
    CaffeinePlus.CaffeinePlus{
        id: caffeinePlus
        onLauncherAdded: {
        	advancedConfig.addApp(url);
        }
    }
}
