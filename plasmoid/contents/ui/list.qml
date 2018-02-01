/*
*   Copyright (C) 2011 by Daker Fernandes Pinheiro <dakerfp@gmail.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Library General Public License as
*   published by the Free Software Foundation; either version 2, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details
*
*   You should have received a copy of the GNU Library General Public
*   License along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

import QtQuick 2.0
import org.kde.plasma.components 2.0


Page {
    tools: ToolBarLayout {
        spacing: 5
        CheckBox {
        	id: enableRestore
            text: "Inhibit suspend"
            onCheckedChanged: {
            	plasmoid.configuration.enableRestore = checked
			}
			onClicked: caffeinePlus.toggle(enableRestore.checked)
			Component.onCompleted: {
				enableRestore.checked = plasmoid.configuration.enableRestore
			}
        }
    }
    ListView {
        id: pageSelector
        clip: true
        anchors.fill: parent

        model:  ListModel {
            id: pagesModel
            ListElement {
                title: "testing title"
            }
            ListElement {
                title: "Checkable buttons"
            }
            ListElement {
                title: "Busy indicators"
            }
            ListElement {
                title: "Sliders"
            }
            ListElement {
                title: "Scrollers"
            }
            ListElement {
                title: "Text elements"
            }
            ListElement {
                title: "Typography"
            }
            ListElement {
                title: "Misc stuff"
            }
        }
        delegate: ListItem {
            enabled: true
            Column {
                Label {
                    text: title
                }
            }
            //onClicked: pageStack.push(Qt.createComponent(page))
        }
    }

    ScrollBar {
        id: horizontalScrollBar

        flickableItem: pageSelector
        orientation: Qt.Horizontal
        anchors {
            left: parent.left
            right: verticalScrollBar.left
            bottom: parent.bottom
        }
    }

    ScrollBar {
        id: verticalScrollBar

        orientation: Qt.Vertical
        flickableItem: pageSelector
        anchors {
            top: parent.top
            right: parent.right
            bottom: horizontalScrollBar.top
        }
    }
}
