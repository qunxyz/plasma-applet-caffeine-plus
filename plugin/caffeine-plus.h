#ifndef CAFFEINE_PLUS_H
#define CAFFEINE_PLUS_H

#include <QUrl>
#include <QDebug>
#include <QFile>
#include <QObject>
#include <QVariantMap>
#include <KWindowInfo>
#include <KWindowSystem>

class QDBusServiceWatcher;

using InhibitionInfo = QPair<QString, QString>;
using InhibitionInfoExtra = QPair<QString, QString>;

#define SUFFIX_USER_APP "|userApps"
#define SUFFIX_FULL_SCREEN "|fullScreen"

class CaffeinePlus : public QObject
{
    Q_OBJECT

public:
	CaffeinePlus(QObject *parent = nullptr);
	virtual ~CaffeinePlus();

	Q_INVOKABLE QVariantMap launcherData(const QUrl &url);
	Q_INVOKABLE void addLauncher(bool isPopup = false);

Q_SIGNALS:
	void launcherAdded(const QString &url, bool isPopup);
	void inhibitionsChanged(bool hasInhibition, int inhibitionSize);

public Q_SLOTS:
	void init(bool enableFullscreen, const QStringList &userApps, bool enableDebug);
	void updateSettings(bool enableFullscreen, const QStringList &userApps, bool enableDebug);
	void checkInhibition();

    bool isInhibited() const {
    	return m_inhibited;
    }

    QVariantMap checkProcessIsInhibited(const QString &id);

    void addInhibition(const QString &appName, const QString &reason);
    void releaseInhibition(const QString &appName);

private Q_SLOTS:
	void inhibitionsChanged(const QList<InhibitionInfo> &added, const QStringList &removed);
	void windowAdded (WId id);
	void windowChanged (WId id);
	void windowRemoved (WId id);

private:
	void checkSysInhibitions();
	void update();
	void inhibitFullscreen(WId id);
	void inhibitUserApps(WId id);
	void listenWindows();
	bool inUserApps(WId id);
	QString getNameByID(const QString &id, bool inhibitType);
	QString getProcessInfo(const QString &id);
	QString plasmaVersion() const;
	void logInfo(QString log_type);

	int getInhibitionIndex(const QString &appName);
	void saveInhibition(InhibitionInfo &item);
	void saveInhibitionExtra(InhibitionInfo &item);
	void saveInhibitionCookie(const QString &appName, const uint cookie);
	void deleteInhibition(const QString &appName);

	QDBusServiceWatcher *m_solidPowerServiceWatcher;
	bool m_serviceRegistered = false;
	bool m_inhibited = false;
	bool m_isInited = false;
	QList<InhibitionInfo> m_apps; // for caffeine, first item is app_name, second is cookie
	QList<InhibitionInfoExtra> m_apps_extra; // for caffeine, first item is app_name, second is app info

	QStringList m_userApps; // for caffeine, custom apps from user configueration
	bool m_enableFullscreen; // for caffeine, determine if enable fullscreen inhibition
	bool m_enableDebug; // for caffeine, determine if enable debug
	QString m_debug_log;
};

#endif // CAFFEINE_PLUS_H
