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

#define SUFFIX_USER_APP "userApps"
#define SUFFIX_FULL_SCREEN "fullScreen"

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

public Q_SLOTS:
	void init(bool enableFullscreen, const QStringList &userApps);
	void updateSettings(bool enableFullscreen, const QStringList &userApps);
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

	QStringList m_userApps; // for caffeine, first item is app_name, second is cookie
	bool m_enableFullscreen; // for caffeine, first item is app_name, second is cookie
};

#endif // CAFFEINE_PLUS_H
