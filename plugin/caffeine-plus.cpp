#include "caffeine-plus.h"

#include <QDebug>
#include <QDataStream>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QMetaObject>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>

#include <QDir>
#include <QFileInfo>
#include <QMimeType>
#include <QMimeDatabase>
#include <QStandardPaths>

#include <KRun>
#include <KConfig>
#include <KConfigGroup>
#include <KFileItem>
#include <KDesktopFile>
#include <KOpenWithDialog>
#include <KPropertiesDialog>

#include <kio/global.h>

static const QString s_solidPowerService = QStringLiteral("org.kde.Solid.PowerManagement.PolicyAgent");
static const QString s_solidPath = QStringLiteral("/org/kde/Solid/PowerManagement/PolicyAgent");

Q_DECLARE_METATYPE(QList<InhibitionInfo>)
Q_DECLARE_METATYPE(InhibitionInfo)

CaffeinePlus::CaffeinePlus(QObject *parent)
    : QObject(parent)
    , m_solidPowerServiceWatcher(new QDBusServiceWatcher(s_solidPowerService, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration))
{
    qDBusRegisterMetaType<QList<InhibitionInfo>>();
    qDBusRegisterMetaType<InhibitionInfo>();

    connect(m_solidPowerServiceWatcher, &QDBusServiceWatcher::serviceUnregistered, this,
        [this] {
            m_serviceRegistered = false;
            m_inhibited = false;
            QDBusConnection::sessionBus().disconnect(s_solidPowerService, s_solidPath, s_solidPowerService,
                                                     QStringLiteral("InhibitionsChanged"),
                                                     this, SLOT(inhibitionsChanged(QList<InhibitionInfo>,QStringList)));
        }
    );
    connect(m_solidPowerServiceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &CaffeinePlus::update);


    // check whether the service is registered
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"),
                                                          QStringLiteral("/"),
                                                          QStringLiteral("org.freedesktop.DBus"),
                                                          QStringLiteral("ListNames"));
    qDebug() << "caffeine-plus::message is: " << message;
    QDBusPendingReply<QStringList> async = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
        [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QStringList> reply = *self;
            self->deleteLater();
            if (!reply.isValid()) {
                return;
            }
            if (reply.value().contains(s_solidPowerService)) {
                update();
            }
        }
    );

    listenWindows();
}

CaffeinePlus::~CaffeinePlus() = default;

void CaffeinePlus::init(bool enableFullscreen, const QStringList &userApps)
{
    qDebug() << "caffeine-plus::init enableFullscreen: " << enableFullscreen;
    qDebug() << "caffeine-plus::init userApps: " << userApps;
    m_userApps = userApps;
    m_enableFullscreen = enableFullscreen;
	m_isInited = true;

}

void CaffeinePlus::update()
{
    qDebug() << "caffeine-plus::update runs";
    m_serviceRegistered = true;
    QDBusConnection::sessionBus().connect(s_solidPowerService, s_solidPath, s_solidPowerService,
                                          QStringLiteral("InhibitionsChanged"),
                                          this, SLOT(inhibitionsChanged(QList<InhibitionInfo>,QStringList)));
    checkInhibition();
}

void CaffeinePlus::inhibitionsChanged(const QList<InhibitionInfo> &added, const QStringList &removed)
{
    qDebug() << "caffeine-plus::inhibitionsChanged runs added count: " << added.count();
	foreach ( InhibitionInfo item, added )
	{
	    qDebug() << "caffeine-plus::inhibitionsChanged runs added item path: " << item.first;
	    qDebug() << "caffeine-plus::inhibitionsChanged runs added item reason: " << item.second;
	}

	foreach ( QString item, removed )
	{
	    qDebug() << "caffeine-plus::inhibitionsChanged runs removed: " << item;
	}
    Q_UNUSED(added)
    Q_UNUSED(removed)

    checkInhibition();
}

void CaffeinePlus::checkInhibition()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(s_solidPowerService,
                                                      s_solidPath,
                                                      s_solidPowerService,
                                                      QStringLiteral("HasInhibition"));
    msg << (uint) 5; // PowerDevil::PolicyAgent::RequiredPolicy::ChangeScreenSettings | PowerDevil::PolicyAgent::RequiredPolicy::InterruptSession
    QDBusPendingReply<bool> pendingReply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(pendingReply, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
        [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<bool> reply = *self;
            self->deleteLater();
            if (!reply.isValid()) {
                return;
            }
            m_inhibited = reply.value();
        }
    );
}

void CaffeinePlus::addInhibition(const QString &appName, const QString &reason)
{
	if ( m_isInited ) {
		QDBusMessage msg = QDBusMessage::createMethodCall(s_solidPowerService,
														  s_solidPath,
														  s_solidPowerService,
														  QStringLiteral("AddInhibition"));
		msg << (uint) 5 << appName << reason; // PowerDevil::PolicyAgent::RequiredPolicy::ChangeScreenSettings | PowerDevil::PolicyAgent::RequiredPolicy::InterruptSession
		QDBusPendingReply<uint> pendingReply = QDBusConnection::sessionBus().asyncCall(msg);
		QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(pendingReply, this);
		connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
			[this, appName](QDBusPendingCallWatcher *self) {
				QDBusPendingReply<uint> reply = *self;
				self->deleteLater();
				if (!reply.isValid()) {
					qDebug() << "Inhibition error: " << reply.error().message();
					return;
				}
				m_apps.append(qMakePair(appName,reply.value()));
			}
		);
	}
}

void CaffeinePlus::releaseInhibition(const QString &appName)
{
	for ( int i=0; i < m_apps.count(); ++i )
	{
		if ( m_apps[i].first == appName ) {
			QDBusMessage msg = QDBusMessage::createMethodCall(s_solidPowerService,
															  s_solidPath,
															  s_solidPowerService,
															  QStringLiteral("ReleaseInhibition"));
			msg << m_apps[i].second;
			QDBusPendingReply<bool> pendingReply = QDBusConnection::sessionBus().asyncCall(msg);
			QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(pendingReply, this);
			connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
				[this](QDBusPendingCallWatcher *self) {
					QDBusPendingReply<bool> reply = *self;
					self->deleteLater();
					if (!reply.isValid()) {
						return;
					}
				}
			);
			m_apps.removeOne(m_apps[i]);
		}
	}
}

//void CaffeinePlus::saveInhibition(const QString &appName, const QString &reason)
//{
//
//}

///////////////////////////////////////////////////////////////////////////////////////////////

void CaffeinePlus::listenWindows()
{
//	QList<WId> windows = KWindowSystem::stackingOrder();
//	int size = windows.count();

    qDebug() << "caffeine-plus::listenWindows ";
	connect(KWindowSystem::self(), SIGNAL(windowAdded(WId)), this, SLOT(windowAdded(WId)));
	connect(KWindowSystem::self(), SIGNAL(windowChanged (WId,NET::Properties,NET::Properties2)), this, SLOT(windowChanged (WId,NET::Properties,NET::Properties2)));
	connect(KWindowSystem::self(), SIGNAL(windowRemoved(WId)), this, SLOT(windowRemoved(WId)));
}

void CaffeinePlus::inhibitFullscreen(WId id, bool isFullScreen)
{
//	QList<WId> windows = KWindowSystem::stackingOrder();
//	int size = windows.count();
	bool needInhibit = isFullScreen;
	QString appName = QString("%1-fullscreen").arg(id);

    qDebug() << "caffeine-plus::inhibitFullscreen ";

    for (int i=0; i < m_apps.count(); ++i)
    {
    	if ( m_apps[i].first == appName ) {
    		needInhibit = false;
    		if ( ! isFullScreen )
    			releaseInhibition(appName);
			break;
    	}
    }
    if ( m_enableFullscreen && needInhibit )
    	addInhibition(appName, QString("inhibit by caffeine plus for fullscreen"));
}

void CaffeinePlus::inhibitUserApps(WId id)
{
	bool needInhibit = true;
	QString appName = QString("%1-userApps").arg(id);

	for (int i=0; i < m_apps.count(); ++i)
	{
		if ( m_apps[i].first == appName ) {
			needInhibit = false;
			break;
		}
	}

	if ( !needInhibit ) return;
	needInhibit = false;

    //KWindowInfo info(id, NET::WMState|NET::WMPid, NET::WM2GroupLeader|NET::WM2WindowClass);
	KWindowInfo info(id, NET::WMState|NET::WMName, NET::WM2GroupLeader|NET::WM2DesktopFileName|NET::WM2WindowClass|NET::WM2WindowRole);

    if (info.valid() ) {
    	WId gid = info.groupLeader();
    	KWindowInfo ginfo(gid, NET::WMState|NET::WMName, NET::WM2DesktopFileName|NET::WM2WindowClass|NET::WM2WindowRole);
        for ( const auto& item : m_userApps  )
        {
        	// Get the name of the file without the extension
        	QString file_name = QFileInfo(item).completeBaseName();
        	QVariantMap item_info = launcherData(item);
        	qDebug() << "caffeine-plus::inhibitUserApps item: " << "applicationName: " << item_info.value("applicationName")
        			<< "genericName: " << item_info.value("genericName") << "exec: " << item_info.value("exec")
					<< "desktop entry: "<< file_name;
        	qDebug() << "caffeine-plus::inhibitUserApps m_userApps: " << "info.name: " << info.name()
        			<< "ginfo.name: " << ginfo.name()
					<< "info desktop entry: " << info.desktopFileName()
					<< "ginfo desktop entry: " << ginfo.desktopFileName()
					<< "info role: " << info.windowRole()
					<< "ginfo role: " << ginfo.windowRole()
					<< "info class: " << info.windowClassClass()
					<< "ginfo class: " << ginfo.windowClassClass()
					<< "info class name: " << info.windowClassName()
					<< "ginfo class name: " << ginfo.windowClassName();
        	if ( item_info.value("applicationName") != "" ) {
        		if ( ginfo.name() != "" && ginfo.name() == item_info.value("applicationName") ) {
        			needInhibit = true;
        			break;
        		}
        	}

        	if (info.desktopFileName() != "" && info.desktopFileName() == file_name) {
    			needInhibit = true;
    			break;
        	}

        	if (item_info.value("exec") != "" && info.windowClassClass() != "" && info.windowClassClass() == item_info.value("exec") ) {
    			needInhibit = true;
    			break;
        	}
        }
    	/////////////////////////////////////
        //QString desktopFileName = QString("%1.desktop").arg(QString(info.desktopFileName()));
/*
        for ( const auto& item : m_userApps  )
        {
        	// Get the name of the file without the extension
        	QString file_name = QFileInfo(item).completeBaseName();
        	QVariantMap item_info = launcherData(item);
            qDebug() << "caffeine-plus::inhibitUserApps m_userApps item: " << info.pid() << file_name << info.groupLeader() << info.windowClassName();
//            if ( desktopFileName == file_name ) {
//                needInhibit = true;
//                break;
//            }
        }
*/
        if ( needInhibit )
        	addInhibition(appName, QString("inhibit by caffeine plus for userApps"));
    } else {
        qDebug() << "caffeine-plus::inhibitUserApps info.valid false:  " << info.name() << id;
    }
}

void CaffeinePlus::windowChanged (WId id, NET::Properties properties, NET::Properties2 properties2)
{
    KWindowInfo info(id, NET::WMState|NET::WMName);
//	KWindowInfo info(id, NET::WMState|NET::WMName|NET::WMDesktop, NET::WM2DesktopFileName);

    qDebug() << "caffeine-plus::windowChanged " << info.name() << id << info.hasState(NET::FullScreen);

    inhibitUserApps(id);

    if (info.valid() ) {
    	inhibitFullscreen(id, info.hasState(NET::FullScreen));
//        if ((m_demandsAttention == 0) && info.hasState(NET::DemandsAttention)) {
//            m_demandsAttention = id;
//            emit windowInAttention(true);
//        } else if ((m_demandsAttention == id) && !info.hasState(NET::DemandsAttention)) {
//            m_demandsAttention = 0;
//            emit windowInAttention(false);
//        }
    } else {
        qDebug() << "caffeine-plus::windowChanged info.valid false:  " << info.name() << id;

    }
}

void CaffeinePlus::windowRemoved (WId id)
{
    qDebug() << "caffeine-plus::windowRemoved " << id;

	bool needUnInhibit = false;
	QString appName = QString("%1-userApps").arg(id);

	for (int i=0; i < m_apps.count(); ++i)
	{
		if ( m_apps[i].first == appName ) {
			needUnInhibit = true;
			break;
		}
	}

    if ( needUnInhibit )
    	releaseInhibition(appName);
}

void CaffeinePlus::windowAdded (WId id)
{
    qDebug() << "caffeine-plus::windowAdded ";
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
QVariantMap CaffeinePlus::launcherData(const QUrl &url)
{
    QString name;
    QString icon;
    QString exec;
    QString genericName;
    QVariantList jumpListActions;

    if (url.scheme() == QLatin1String("quicklaunch")) {
        // Ignore internal scheme
    } else if (url.isLocalFile()) {
        const KFileItem fileItem(url);
        const QFileInfo fi(url.toLocalFile());

        if (fileItem.isDesktopFile()) {
            const KDesktopFile f(url.toLocalFile());
            name = f.readName();
            icon = f.readIcon();
            genericName = f.readGenericName();
            if (name.isEmpty()) {
                name = QFileInfo(url.toLocalFile()).fileName();
            }
            if (f.desktopGroup().hasKey("Exec")) {
            	exec = f.desktopGroup().readPathEntry( "Exec", QString() );
            }

            const QStringList &actions = f.readActions();

            foreach (const QString &actionName, actions) {
                const KConfigGroup &actionGroup = f.actionGroup(actionName);

                if (!actionGroup.isValid() || !actionGroup.exists()) {
                    continue;
                }

                const QString &name = actionGroup.readEntry("Name");
                const QString &exec = actionGroup.readEntry("Exec");
                if (name.isEmpty() || exec.isEmpty()) {
                    continue;
                }

                jumpListActions << QVariantMap{
                    {QStringLiteral("name"), name},
                    {QStringLiteral("icon"), actionGroup.readEntry("Icon")},
                    {QStringLiteral("exec"), exec}
                };
            }
        } else {
            QMimeDatabase db;
            name = fi.baseName();
            icon = db.mimeTypeForUrl(url).iconName();
            genericName = fi.baseName();
        }
    } else {
        if (url.scheme().contains(QLatin1String("http"))) {
            name = url.host();
        } else if (name.isEmpty()) {
            name = url.toString();
            if (name.endsWith(QLatin1String(":/"))) {
                name = url.scheme();
            }
        }
        icon = KIO::iconNameForUrl(url);
    }

    return QVariantMap{
        {QStringLiteral("applicationName"), name},
        {QStringLiteral("iconName"), icon},
        {QStringLiteral("genericName"), genericName},
        {QStringLiteral("exec"), exec},
        {QStringLiteral("jumpListActions"), jumpListActions}
    };
}

void CaffeinePlus::openUrl(const QUrl &url)
{
    new KRun(url, Q_NULLPTR);
}

void CaffeinePlus::openExec(const QString &exec)
{
    KRun::run(exec, {}, Q_NULLPTR);
}

void CaffeinePlus::addLauncher(bool isPopup)
{
    KOpenWithDialog *dialog = new KOpenWithDialog();
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->hideRunInTerminal();
    dialog->setSaveNewApplications(true);
    dialog->show();

    connect(dialog, &KOpenWithDialog::accepted, this, [this, dialog, isPopup]() {
        if (!dialog->service()) {
            return;
        }
        const QUrl &url = QUrl::fromLocalFile(dialog->service()->entryPath());
        if (url.isValid()) {
            Q_EMIT launcherAdded(url.toString(), isPopup);
        }
    });
}

