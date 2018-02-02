import QtQuick.Controls 1.3
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaComponents.ButtonRow {
    exclusive: true
    spacing: 0

    ToolButton {
    	visible: false
    }
    ToolButton {
    	text: "Remove"
    	onClicked: {
    		var index = advancedConfig.cfg_userApps.indexOf(parent.children[0].text)
    		advancedConfig.cfg_userApps.splice(index, 1)
			advancedConfig.configurationChanged()
    		parent.destroy()
    	}
    }
}