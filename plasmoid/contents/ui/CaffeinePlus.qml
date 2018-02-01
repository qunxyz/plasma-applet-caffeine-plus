import QtQuick 2.2

Item {
    id: caffeinePlus

    property var plugin: null
    property bool failedToInitialize: false

    function getCaffeinePlus() {

        if (plugin !== null) {
            return plugin
        }

        if (!failedToInitialize) {
            console.log('Initializing Caffeine Plus...')
            try {
                plugin = Qt.createQmlObject('import org.kde.private.CaffeinePlus 1.0 as CP; CP.CaffeinePlus {}', caffeinePlus, 'CaffeinePlus')
                console.log('Caffeine Plus initialized successfully!')
            }catch (e) {
                console.exception('ERROR: Caffeine Plus FAILED to initialize -->', e)
                failedToInitialize = true
            }
        }

        return plugin
    }

    function getInstance() {
        var c = getCaffeinePlus()
        if (c) {
            return c
        } else {
            console.exception('ERROR: get name from plugin - Caffeine Plus not available')
        }
    }

    function toggle(flag) {
        var inst = getInstance()
        if (flag) {
            inst.addInhibition("user", "inhibit by caffeine plus")
        } else {
            inst.releaseInhibition("user")
        }
    }
}