#ifndef CAFFEINE_PLUS_H
#define CAFFEINE_PLUS_H

#include <QUrl>
#include <QDebug>
#include <QObject>
#include <QVariantMap>
#include <KWindowInfo>
#include <KWindowSystem>

class QDBusServiceWatcher;

using InhibitionInfo = QPair<QString, QString>;
using InhibitionInfoCaffeinePlus = QPair<QString, uint>;

#define SUFFIX_USER_APP "userApps"
#define SUFFIX_FULL_SCREEN "fullscreen"

class CaffeinePlus : public QObject
{
    Q_OBJECT

public:
	CaffeinePlus(QObject *parent = nullptr);
	virtual ~CaffeinePlus();

	///////////////////////////////////////////////////

	Q_INVOKABLE QVariantMap launcherData(const QUrl &url);
	Q_INVOKABLE void openUrl(const QUrl &url);
	Q_INVOKABLE void openExec(const QString &exec);

	Q_INVOKABLE void addLauncher(bool isPopup = false);

Q_SIGNALS:
	void launcherAdded(const QString &url, bool isPopup);
	///////////////////////////////////////////////////

public Q_SLOTS:
	void init(bool enableFullscreen, const QStringList &userApps);
	void checkInhibition();

    bool isInhibited() const {
//    	qDebug() << "caffeine-plus::isInhibited m_inhibited: " << m_inhibited;
//    	qDebug() << "caffeine-plus::isInhibited m_apps: " << m_apps;
    	//return m_inhibited;
    	if (m_apps.count()) return true;

    	return false;
    }

    QVariantMap checkProcessIsInhibited(const QString &id);

    void addInhibition(const QString &appName, const QString &reason);
    void releaseInhibition(const QString &appName);

private Q_SLOTS:
	void inhibitionsChanged(const QList<InhibitionInfo> &added, const QStringList &removed);
	void windowAdded (WId id);
	void windowChanged (WId id, NET::Properties properties, NET::Properties2 properties2);
	void windowRemoved (WId id);

private:
//	void checkInhibition();
	void update();
	void inhibitFullscreen(WId id);
	void inhibitUserApps(WId id);
	void listenWindows();
	QString getNameByID(const QString &id, bool inhibitType);

	QDBusServiceWatcher *m_solidPowerServiceWatcher;
	bool m_serviceRegistered = false;
	bool m_inhibited = false;
	bool m_isInited = false;
	QList<InhibitionInfo> m_apps; // for caffeine, first item is app_name, second is cookie
	QList<InhibitionInfo> m_sys_apps; // for caffeine, first item is app_name, second is cookie

	QStringList m_userApps; // for caffeine, first item is app_name, second is cookie
	bool m_enableFullscreen; // for caffeine, first item is app_name, second is cookie
};

#endif // CAFFEINE_PLUS_H
