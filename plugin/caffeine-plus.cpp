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
#include <KCoreAddons>

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

void CaffeinePlus::init(bool enableFullscreen, const QStringList &userApps, bool enableDebug)
{
    m_userApps = userApps;
    m_enableFullscreen = enableFullscreen;
    m_enableDebug = enableDebug;
    m_isInited = true;
    m_debug_log = QString("%1/caffeine-plus-debug.log").arg(QStandardPaths::CacheLocation);
    logInfo("system");

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
    checkSysInhibitions();
}
void CaffeinePlus::updateSettings(bool enableFullscreen, const QStringList &userApps, bool enableDebug)
{
    m_userApps = userApps;
    m_enableFullscreen = enableFullscreen;
    m_enableDebug = enableDebug;
    logInfo("system");

    for ( int i = 0; i < m_apps.count(); ++i ) {
    	bool isCookie;

		uint cookie = m_apps[i].second.toLong(&isCookie, 10);
		deleteInhibition(m_apps[i].first);
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

	QList<WId> windows = KWindowSystem::windows();
	for (auto it = windows.cbegin(), end = windows.cend(); it != end; ++it)
		windowChanged (*it);
}

void CaffeinePlus::logInfo(QString log_type)
{
	if (!m_enableDebug)
		return;

	QString process_info = "";
	QFile out_log(m_debug_log);
	bool iswriteable = true;

	if (!out_log.open(QIODevice::Append | QIODevice::Text))
		iswriteable = false;

	QTextStream out(&out_log);

	if (log_type == "system") {
		QString os_file_path = "";
		if (QFile::exists(QStringLiteral("/etc/os-release"))) {
			os_file_path = "/etc/os-release";
		} else if (QFile::exists(QStringLiteral("/usr/lib/os-release"))) {
			os_file_path = "/usr/lib/os-release";
		}

		out << "KDE Plasma Version: " << plasmaVersion() << "\n";
		out << "KDE Frameworks Version: " << KCoreAddons::versionString() << "\n";
		out << "Qt Version: " << QString::fromLatin1(qVersion()) << "\n";

		QFile os_file(os_file_path);
		if (!os_file.open(QIODevice::ReadOnly | QIODevice::Text))
			return;

		while (!os_file.atEnd()) {
			out << os_file.readLine();
		}
	}

	if (log_type == "apps") {
		for ( int i = 0; i < m_apps.count(); ++i ) {
			QStringList pieces = m_apps[i].first.split( "|" );
			if (iswriteable) {
				out << "caffeine-plus::checkInhibition debug: "
					<< m_apps[i].first << " " << m_apps_extra[i].second << " ";
			}
			if (pieces.length() > 1) {
				process_info = getProcessInfo(QString("%1").arg(pieces.value(0)));
				if (process_info == "" && iswriteable)
					out << "This inhibition is dead, should be removed!\n";
				else if (iswriteable)
					out << " " << process_info << "\n";
			} else if (iswriteable)
				out << " " << process_info << "\n";
		}
	}
}

void CaffeinePlus::checkSysInhibitions()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(s_solidPowerService,
                                                      s_solidPath,
                                                      s_solidPowerService,
                                                      QStringLiteral("ListInhibitions"));

    QDBusPendingReply<QList<InhibitionInfo>> pendingReply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(pendingReply, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
        [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QList<InhibitionInfo>> reply = *self;
            self->deleteLater();
            if (!reply.isValid()) {
				qDebug() << "caffeine-plus::checkSysInhibitions error: " << reply.error().message();
                return;
            }

            for ( int index = 0; index < reply.value().count(); ++index ) {
            	InhibitionInfo item = reply.value()[index];
            	InhibitionInfoExtra item_extra;
            	QStringList pieces = item.first.split( "|" );
            	if (pieces.length() > 1)
            		item_extra = qMakePair(item.first, item.second);
            	else
            		item_extra = qMakePair(item.first, QString("%1 (from system)").arg(item.second));;
            	saveInhibition(item);
            	saveInhibitionExtra(item_extra);
            }
        }
    );
}

void CaffeinePlus::update()
{
    m_serviceRegistered = true;
    QDBusConnection::sessionBus().connect(s_solidPowerService, s_solidPath, s_solidPowerService,
                                          QStringLiteral("InhibitionsChanged"),
                                          this, SLOT(inhibitionsChanged(QList<InhibitionInfo>,QStringList)));
    checkInhibition();
}

QString CaffeinePlus::plasmaVersion() const
{
    const QStringList &filePaths = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                             QStringLiteral("xsessions/plasma.desktop"));

    if (filePaths.length() < 1) {
        return QString();
    }

    // Despite the fact that there can be multiple desktop files we simply take
    // the first one as users usually don't have xsessions/ in their $HOME
    // data location, so the first match should (usually) be the only one and
    // reflect the plasma session run.
    KDesktopFile desktopFile(filePaths.first());
    return desktopFile.desktopGroup().readEntry("X-KDE-PluginInfo-Version", QString());
}

void CaffeinePlus::inhibitionsChanged(const QList<InhibitionInfo> &added, const QStringList &removed)
{
	foreach ( InhibitionInfo item, added ) {
		InhibitionInfoExtra item_extra = qMakePair(item.first,item.second);
    	saveInhibition(item);
    	saveInhibitionExtra(item_extra);
	}

	foreach ( QString item, removed )
		deleteInhibition(item);

    Q_UNUSED(added)
    Q_UNUSED(removed)

    checkInhibition();
}

void CaffeinePlus::checkInhibition()
{
    logInfo("apps");
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
            Q_EMIT inhibitionsChanged(m_inhibited, m_apps.count());
        }
    );
}

void CaffeinePlus::addInhibition(const QString &appName, const QString &reason)
{
	if ( ! m_isInited ) return;
	if (getInhibitionIndex(appName) >= 0) return;

	InhibitionInfo item = qMakePair(appName,reason);
	InhibitionInfoExtra item_extra = qMakePair(appName,reason);

	saveInhibition(item); // insert first
	saveInhibitionExtra(item_extra); // insert first

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
				deleteInhibition(appName);

				return;
			}

			saveInhibitionCookie(appName, reply.value());
		}
	);
}

void CaffeinePlus::releaseInhibition(const QString &appName)
{
	int index = getInhibitionIndex(appName);
	if (index < 0) return;
	InhibitionInfo item = m_apps[index];

	bool isCookie;

	uint cookie = item.second.toLong(&isCookie, 10);
	if (!isCookie){
		qDebug() << "caffeine-plus::releaseInhibition cookie error second: " << item.second;
		return;
	}

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

			deleteInhibition(item.first);
		}
	);
}

int CaffeinePlus::getInhibitionIndex(const QString &appName)
{
	for ( int i = 0; i < m_apps.count(); ++i )
		if ( m_apps[i].first == appName )
			return i;

	return -1;
}

void CaffeinePlus::saveInhibition(InhibitionInfo &item)
{
	bool needInsert = true;
	for ( int i = 0; i < m_apps.count(); ++i )
		if ( m_apps[i].first == item.first ) {
			needInsert = false;
			break;
		}

	if (!needInsert) return;

	m_apps.append(qMakePair(item.first,item.second));
}

void CaffeinePlus::saveInhibitionExtra(InhibitionInfoExtra &item)
{
	bool needInsert = true;
	for ( int i = 0; i < m_apps_extra.count(); ++i )
		if ( m_apps_extra[i].first == item.first ) {
			needInsert = false;
			break;
		}

	if (!needInsert) return;

	m_apps_extra.append(qMakePair(item.first,item.second));
}

void CaffeinePlus::saveInhibitionCookie(const QString &appName, const uint cookie)
{
	for ( int i = 0; i < m_apps.count(); ++i )
		if ( m_apps[i].first == appName ) {
			m_apps[i].second = QString("%1").arg(cookie);
			return;
		}
}

void CaffeinePlus::deleteInhibition(const QString &appName)
{
    for ( int i = 0; i < m_apps.count(); ++i ) {
		if ( m_apps[i].first == appName ) {
			m_apps.removeOne(m_apps[i]);
			m_apps_extra.removeOne(m_apps_extra[i]);
		}
    }
}

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

	if (getInhibitionIndex(appName) >= 0) needInhibit = false;

    if ( m_enableFullscreen && needInhibit )
    	addInhibition(appName, QString("inhibit by caffeine plus for fullscreen"));
    else if (!isFullScreen)
    	releaseInhibition(appName);
}

bool CaffeinePlus::inUserApps(WId id)
{
	bool isInUserApps = false;
	KWindowInfo info(id, NET::WMState|NET::WMName, NET::WM2GroupLeader|NET::WM2DesktopFileName|NET::WM2WindowClass|NET::WM2WindowRole);
	if (!info.valid() ) return isInUserApps;

	WId gid = info.groupLeader();
	KWindowInfo ginfo(gid, NET::WMState|NET::WMName, NET::WM2DesktopFileName|NET::WM2WindowClass|NET::WM2WindowRole);
    for ( const auto& item : m_userApps  )
    {
    	// Get the name of the file without the extension
    	QString file_name = QFileInfo(item).completeBaseName();
    	QVariantMap item_info = launcherData(item);
    	QString applicationName = item_info.value("applicationName").toString();
    	QString exec = item_info.value("exec").toString();

    	if ( applicationName != "" ) {
    		if ( file_name.indexOf(applicationName, 0) >= 0 || ( ginfo.name() != "" && applicationName.indexOf(ginfo.name(), 0) >= 0 ) ) {
    			isInUserApps = true;
    			break;
    		}
    	}

    	if ( info.desktopFileName() != "" && file_name.indexOf(info.desktopFileName(), 0) >= 0 ) {
			isInUserApps = true;
			break;
    	}

    	if ( item_info.value("exec") != "" && info.windowClassClass() != "" && exec.indexOf(info.windowClassClass(), 0) >= 0 ) {
			isInUserApps = true;
			break;
    	}
    }

    return isInUserApps;
}

void CaffeinePlus::inhibitUserApps(WId id)
{
	QString appName = getNameByID(QString("%1").arg(id), true);

	if (getInhibitionIndex(appName) >= 0 && !inUserApps(id)) {
		releaseInhibition(appName);
		return;
	}

    if ( inUserApps(id) )
    	addInhibition(appName, QString("inhibit by caffeine plus for userApps"));
}

QString CaffeinePlus::getNameByID(const QString &id, bool inhibitType) {
	if (inhibitType)
		return QString("%1%2").arg(id).arg(SUFFIX_USER_APP);

	return QString("%1%2").arg(id).arg(SUFFIX_FULL_SCREEN);
}

void CaffeinePlus::windowChanged (WId id)
{
	inhibitFullscreen(id);
	inhibitUserApps(id);
}

void CaffeinePlus::windowRemoved (WId id)
{
	QString appNameUserApps = getNameByID(QString("%1").arg(id), true);
	QString appNameFullScreen = getNameByID(QString("%1").arg(id), false);

	if (getInhibitionIndex(appNameUserApps) >= 0)
    	releaseInhibition(appNameUserApps);

	if (getInhibitionIndex(appNameFullScreen) >= 0)
    	releaseInhibition(appNameFullScreen);
}

void CaffeinePlus::windowAdded (WId id)
{
	inhibitFullscreen(id);
	inhibitUserApps(id);
}

QString CaffeinePlus::getProcessInfo(const QString &id) {
    //QString appNameSys = "";
	QString msg = "";
	bool isWID;
	WId wid = id.toLong(&isWID, 10);

	if (isWID) {
		KWindowInfo info(wid, NET::WMState|NET::WMName|NET::WMPid,
				NET::WM2GroupLeader|NET::WM2DesktopFileName|NET::WM2WindowClass|NET::WM2WindowRole);
		if (info.valid() ) {
			msg = QString("Name: %1  PID: %2  DesktopFileName: %3")
					.arg(info.windowClassClass(), QString::number(info.pid()), QString(info.desktopFileName()));
		}
	}

	return msg;
}

QVariantMap CaffeinePlus::checkProcessIsInhibited(const QString &id) {
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

	if (getInhibitionIndex(appNameUserApps) >= 0)
		inhibitedUserApps = true;

	if (getInhibitionIndex(appNameFullScreen) >= 0)
		inhibitedFullScreen = true;

	if (getInhibitionIndex(appNameSys) >= 0)
		inhibitedSys = true;

    return QVariantMap{
        {QStringLiteral("inhibitedUserApps"), inhibitedUserApps},
        {QStringLiteral("inhibitedFullScreen"), inhibitedFullScreen},
        {QStringLiteral("inhibitedSys"), inhibitedSys}
    };
}

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
