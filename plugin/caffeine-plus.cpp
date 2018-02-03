#include "caffeine-plus.h"

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
}

CaffeinePlus::~CaffeinePlus() = default;

void CaffeinePlus::init(bool enableFullscreen, const QStringList &userApps)
{
    m_userApps = userApps;
    m_enableFullscreen = enableFullscreen;
	m_isInited = true;

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

    QDBusPendingReply<QStringList> async = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
        [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QStringList> reply = *self;
            self->deleteLater();
            if (!reply.isValid()) {
				qDebug() << "caffeine-plus::init error: " << reply.error().message();
                return;
            }
            if (reply.value().contains(s_solidPowerService)) {
                update();
            }
        }
    );

    listenWindows();

}
void CaffeinePlus::updateSettings(bool enableFullscreen, const QStringList &userApps)
{
	qDebug() << "caffeine-plus::updateSettings";
    m_userApps = userApps;
    m_enableFullscreen = enableFullscreen;

//    QList<WId> windows = KWindowSystem::windows();
//    for (auto it = windows.cbegin(), end = windows.cend(); it != end; ++it) {
//    	windowChanged (*it);
//    }

    for ( int i = 0; i < m_apps.count(); ++i ) {
    	bool isCookie;

		uint cookie = m_apps[i].second.toLong(&isCookie, 10);
		m_apps.removeOne(m_apps[i]);
		if (!isCookie) continue;

		QDBusMessage msg = QDBusMessage::createMethodCall(s_solidPowerService,
														  s_solidPath,
														  s_solidPowerService,
														  QStringLiteral("ReleaseInhibition"));
		msg << cookie;
		QDBusPendingReply<void> pendingReply = QDBusConnection::sessionBus().asyncCall(msg);
		QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(pendingReply, this);
		connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
			[this](QDBusPendingCallWatcher *self) {
				QDBusPendingReply<void> reply = *self;
				self->deleteLater();
				if (!reply.isValid()) {
					qDebug() << "caffeine-plus::releaseInhibition error: " << reply.error().message();
					return;
				}
			}
		);
    }

//    init(enableFullscreen, userApps);
	QList<WId> windows = KWindowSystem::windows();
	for (auto it = windows.cbegin(), end = windows.cend(); it != end; ++it) {
		windowChanged (*it);
	}
}

void CaffeinePlus::update()
{
    m_serviceRegistered = true;
    QDBusConnection::sessionBus().connect(s_solidPowerService, s_solidPath, s_solidPowerService,
                                          QStringLiteral("InhibitionsChanged"),
                                          this, SLOT(inhibitionsChanged(QList<InhibitionInfo>,QStringList)));
    checkInhibition();
}

void CaffeinePlus::inhibitionsChanged(const QList<InhibitionInfo> &added, const QStringList &removed)
{
	foreach ( InhibitionInfo item, added )
	{
		bool needInsert = true;
		for ( int i = 0; i < m_apps.count(); ++i )
			if ( m_apps[i].first == item.first ) {
				needInsert = false;
				break;
			}

		if (!needInsert) continue;

		m_apps.append(qMakePair(item.first,item.second));
	}

	foreach ( QString item, removed )
	    for ( int i = 0; i < m_apps.count(); ++i )
			if ( m_apps[i].first == item )
				m_apps.removeOne(m_apps[i]);

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
				qDebug() << "caffeine-plus::checkInhibition error: " << reply.error().message();
                return;
            }
            m_inhibited = reply.value();
        }
    );
}

void CaffeinePlus::addInhibition(const QString &appName, const QString &reason)
{
	if ( ! m_isInited ) return;
	for ( int i=0; i < m_apps.count(); ++i )
		if ( m_apps[i].first == appName ) return;

	m_apps.append(qMakePair(appName,reason)); // insert first

	QDBusMessage msg = QDBusMessage::createMethodCall(s_solidPowerService,
													  s_solidPath,
													  s_solidPowerService,
													  QStringLiteral("AddInhibition"));
	msg << (uint) 5 << appName << reason;
	QDBusPendingReply<uint> pendingReply = QDBusConnection::sessionBus().asyncCall(msg);
	QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(pendingReply, this);
	connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
		[this, appName](QDBusPendingCallWatcher *self) {
			QDBusPendingReply<uint> reply = *self;
			self->deleteLater();
			if (!reply.isValid()) {
				qDebug() << "caffeine-plus::addInhibition error: " << reply.error().message();
				for ( int i = 0; i < m_apps.count(); ++i )
					if ( m_apps[i].first == appName ) {
						m_apps.removeOne(m_apps[i]);
						return;
					}
				return;
			}
			for ( int i = 0; i < m_apps.count(); ++i )
				if ( m_apps[i].first == appName ) {
					m_apps[i].second = QString("%1").arg(reply.value());
					return;
				}
		}
	);
}

void CaffeinePlus::releaseInhibition(const QString &appName)
{
	for ( int i=0; i < m_apps.count(); ++i )
	{
		if ( m_apps[i].first == appName ) {
			bool isCookie;

			uint cookie = m_apps[i].second.toLong(&isCookie, 10);
			if (!isCookie){
				qDebug() << "caffeine-plus::releaseInhibition cookie error second: " << m_apps[i].second;
				return;
			}
			InhibitionInfo item = m_apps[i];

			QDBusMessage msg = QDBusMessage::createMethodCall(s_solidPowerService,
															  s_solidPath,
															  s_solidPowerService,
															  QStringLiteral("ReleaseInhibition"));
			msg << cookie;
			QDBusPendingReply<void> pendingReply = QDBusConnection::sessionBus().asyncCall(msg);
			QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(pendingReply, this);
			connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
				[this, item](QDBusPendingCallWatcher *self) {
					QDBusPendingReply<void> reply = *self;
					self->deleteLater();
					if (!reply.isValid()) {
						qDebug() << "caffeine-plus::releaseInhibition error: " << reply.error().message();
						return;
					}

					m_apps.removeOne(item);
				}
			);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CaffeinePlus::listenWindows()
{
	connect(KWindowSystem::self(), SIGNAL(windowAdded(WId)), this, SLOT(windowAdded(WId)));
	connect(KWindowSystem::self(), SIGNAL(windowChanged (WId)), this, SLOT(windowChanged (WId)));
	connect(KWindowSystem::self(), SIGNAL(windowRemoved(WId)), this, SLOT(windowRemoved(WId)));
}

void CaffeinePlus::inhibitFullscreen(WId id)
{
	KWindowInfo info(id, NET::WMState);
	if (!info.valid() ) return;

	bool isFullScreen = info.hasState(NET::FullScreen);
	bool needInhibit = isFullScreen;
	QString appName = getNameByID(QString("%1").arg(id), false);

    for (int i=0; i < m_apps.count(); ++i)
    	if ( m_apps[i].first == appName ) {
    		needInhibit = false;

			break;
    	}

    if ( m_enableFullscreen && needInhibit )
    	addInhibition(appName, QString("inhibit by caffeine plus for fullscreen"));
    else if (!isFullScreen)
    	releaseInhibition(appName);
}

void CaffeinePlus::inhibitUserApps(WId id)
{
	bool needInhibit = true;
	QString appName = getNameByID(QString("%1").arg(id), true);

	for (int i=0; i < m_apps.count(); ++i)
		if ( m_apps[i].first == appName ) {
			needInhibit = false;
			break;
		}

	if ( !needInhibit ) return;
	needInhibit = false;

	KWindowInfo info(id, NET::WMState|NET::WMName, NET::WM2GroupLeader|NET::WM2DesktopFileName|NET::WM2WindowClass|NET::WM2WindowRole);
	if (!info.valid() ) return;

	WId gid = info.groupLeader();
	KWindowInfo ginfo(gid, NET::WMState|NET::WMName, NET::WM2DesktopFileName|NET::WM2WindowClass|NET::WM2WindowRole);
    for ( const auto& item : m_userApps  )
    {
    	// Get the name of the file without the extension
    	QString file_name = QFileInfo(item).completeBaseName();
    	QVariantMap item_info = launcherData(item);
    	/*
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
				<< "ginfo class name: " << ginfo.windowClassName();*/
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

    if ( needInhibit )
    	addInhibition(appName, QString("inhibit by caffeine plus for userApps"));
}

QString CaffeinePlus::getNameByID(const QString &id, bool inhibitType) {
	if (inhibitType)
		return QString("%1-%2").arg(id).arg(SUFFIX_USER_APP);

	return QString("%1-%2").arg(id).arg(SUFFIX_FULL_SCREEN);
}

void CaffeinePlus::windowChanged (WId id)
{
	inhibitFullscreen(id);
	inhibitUserApps(id);
}

void CaffeinePlus::windowRemoved (WId id)
{
	bool needUnInhibitUserApps = false;
	bool needUnInhibitFullScreen = false;
	QString appNameUserApps = getNameByID(QString("%1").arg(id), true);
	QString appNameFullScreen = getNameByID(QString("%1").arg(id), false);

	for (int i=0; i < m_apps.count(); ++i)
	{
		if ( m_apps[i].first == appNameUserApps )
			needUnInhibitUserApps = true;

		if ( m_apps[i].first == appNameFullScreen )
			needUnInhibitFullScreen = true;
	}

    if ( needUnInhibitUserApps )
    	releaseInhibition(appNameUserApps);

    if ( needUnInhibitFullScreen )
    	releaseInhibition(appNameFullScreen);
}

void CaffeinePlus::windowAdded (WId id)
{
    qDebug() << "caffeine-plus::windowAdded ";
	inhibitFullscreen(id);
	inhibitUserApps(id);
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
QVariantMap CaffeinePlus::checkProcessIsInhibited(const QString &id) {
    qDebug() << "caffeine-plus::checkProcessIsInhibited " << id;
    QString appNameSys = "";
	QString appNameUserApps = getNameByID(id, true);
	QString appNameFullScreen = getNameByID(id, false);
	bool inhibitedUserApps = false;
	bool inhibitedFullScreen = false;
	bool inhibitedSys = false;

	bool isWID;

	WId wid = id.toLong(&isWID, 10);

	if (isWID) {
		KWindowInfo info(wid, NET::WMState|NET::WMName, NET::WM2GroupLeader|NET::WM2DesktopFileName|NET::WM2WindowClass|NET::WM2WindowRole);
		if (info.valid() )
			appNameSys = info.windowClassClass().toLower();
	}

	for (int i = 0; i < m_apps.count(); ++i) {
		if ( m_apps[i].first == appNameUserApps )
			inhibitedUserApps = true;

		if ( m_apps[i].first == appNameFullScreen )
			inhibitedFullScreen = true;

		if ( m_apps[i].first.toLower() == appNameSys )
			inhibitedSys = true;
	}

    return QVariantMap{
        {QStringLiteral("inhibitedUserApps"), inhibitedUserApps},
        {QStringLiteral("inhibitedFullScreen"), inhibitedFullScreen},
        {QStringLiteral("inhibitedSys"), inhibitedSys}
    };
}
///////////////////////////////////////////////////////
QVariantMap CaffeinePlus::launcherData(const QUrl &url)
{
    QString name;
    QString icon;
    QString exec;
    QString genericName;
    QVariantList jumpListActions;

    if (url.isLocalFile()) {
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

