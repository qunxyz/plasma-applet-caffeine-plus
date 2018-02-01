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
    if ( needInhibit )
    	addInhibition(appName, QString("inhibit by caffeine plus for fullscreen"));
}

void CaffeinePlus::windowChanged (WId id, NET::Properties properties, NET::Properties2 properties2)
{
    KWindowInfo info(id, NET::WMState|NET::WMName);
//	KWindowInfo info(id, NET::WMState|NET::WMName|NET::WMDesktop, NET::WM2DesktopFileName);

    qDebug() << "caffeine-plus::windowChanged " << info.name() << id << info.hasState(NET::FullScreen);

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
}

void CaffeinePlus::windowAdded (WId id)
{
    qDebug() << "caffeine-plus::windowAdded ";
}
///////////////////////////////////////////////////////
