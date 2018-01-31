#include "plasmoidplugin.h"
#include "caffeine-plus.h"

#include <QtQml>
#include <QDebug>

void PlasmoidPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.private.CaffeinePlus"));

    qmlRegisterType<CaffeinePlus>(uri, 1, 0, "CaffeinePlus");
}
